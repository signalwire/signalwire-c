/*
 * Copyright (c) 2018-2019 SignalWire, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "signalwire-client-c/client.h"

#define ks_time_now_ms() ks_time_ms(ks_time_now())

static void deregister_cmd(swclt_conn_t *ctx, ks_uuid_t id);

/* 1638 commands per second over 5 second average TTL */
#define TTL_HEAP_MAX_SIZE 8192

typedef struct swclt_ttl_node {
	ks_time_t expiry;
	ks_uuid_t id;
} swclt_ttl_node_t;

struct swclt_ttl_tracker {
	swclt_ttl_node_t heap[TTL_HEAP_MAX_SIZE]; // min heap of TTLs to expire
	int count;
	ks_cond_t *cond;
	ks_thread_t *thread;
	swclt_conn_t *conn;
};

#define TTL_HEAP_ROOT 0
#define TTL_HEAP_PARENT(pos) ((pos - 1) / 2)
#define TTL_HEAP_LEFT_CHILD(pos) ((pos * 2) + 1)
#define TTL_HEAP_RIGHT_CHILD(pos) ((pos * 2) + 2)

inline static void ttl_heap_swap(swclt_ttl_tracker_t *ttl, int pos1, int pos2)
{
	if (pos1 == pos2) {
		return;
	}
	swclt_ttl_node_t tmp = ttl->heap[pos1];
	ttl->heap[pos1] = ttl->heap[pos2];
	ttl->heap[pos2] = tmp;
}

static ks_status_t ttl_heap_remove(swclt_ttl_tracker_t *ttl)
{
	if (ttl->count <= 0) {
		return KS_STATUS_FAIL;
	}
	// clear entry at root and swap with last entry
	memset(&ttl->heap[TTL_HEAP_ROOT], 0, sizeof(ttl->heap[TTL_HEAP_ROOT]));
	int pos = ttl->count - 1;
	ttl->count--;
	ttl_heap_swap(ttl, pos, TTL_HEAP_ROOT);

	// sift down the value...
	while (pos < ttl->count) {
		int swap = TTL_HEAP_LEFT_CHILD(pos);
		int right = TTL_HEAP_RIGHT_CHILD(pos);
		// if there is no left child or there is a right child and it is higher priority than left
		if (!ttl->heap[swap].expiry || (ttl->heap[right].expiry && ttl->heap[right].expiry < ttl->heap[swap].expiry)) {
			swap = right;
		}
		if (ttl->heap[swap].expiry && ttl->heap[pos].expiry > ttl->heap[swap].expiry) {
			ttl_heap_swap(ttl, pos, swap);
			pos = swap;
		} else {
			// done
			break;
		}
	}
	return KS_STATUS_SUCCESS;
}

static ks_status_t ttl_heap_insert(swclt_ttl_tracker_t *ttl, ks_time_t expiry, ks_uuid_t id)
{
	if (ttl->count >= TTL_HEAP_MAX_SIZE) {
		return KS_STATUS_FAIL;
	}

	if (expiry == 0) {
		return KS_STATUS_FAIL;
	}

	// add to last position in the heap
	int pos = ttl->count;
	ttl->count++;
	ttl->heap[pos].expiry = expiry;
	ttl->heap[pos].id = id;

	// now sift up the value
	while (pos > TTL_HEAP_ROOT) {
		int parent = TTL_HEAP_PARENT(pos);
		if (ttl->heap[parent].expiry > expiry) {
			ttl_heap_swap(ttl, parent, pos);
			pos = parent;
		} else {
			break;
		}
	}
	return KS_STATUS_SUCCESS;
}

static ks_status_t ttl_tracker_watch(swclt_ttl_tracker_t *ttl, ks_time_t expiry, ks_uuid_t id)
{
	ks_cond_lock(ttl->cond);
	// need to wake thread if this TTL is before the next one
	int wake_ttl_tracker_thread = !ttl->heap[TTL_HEAP_ROOT].expiry || ttl->heap[TTL_HEAP_ROOT].expiry > expiry;
	if (ttl_heap_insert(ttl, expiry, id) != KS_STATUS_SUCCESS) {
		ks_cond_unlock(ttl->cond);
		ks_log(KS_LOG_ERROR, "Failed to track command %s TTL", ks_uuid_thr_str(&id));
		return KS_STATUS_FAIL;
	}
	if (wake_ttl_tracker_thread) {
		// notify of new shortest TTL...
		ks_cond_broadcast(ttl->cond);
	}
	ks_cond_unlock(ttl->cond);
	return KS_STATUS_SUCCESS;
}

static ks_status_t ttl_tracker_next(swclt_ttl_tracker_t *ttl, ks_uuid_t *id)
{
	ks_cond_lock(ttl->cond);
	ks_time_t wait_ms = 0;
	ks_time_t now_ms = ks_time_now_ms();

	// how long to wait for next TTL expiration?
	if (!ttl->heap[TTL_HEAP_ROOT].expiry) {
		// nothing to wait for
		wait_ms = 5000;
	} else if (ttl->heap[TTL_HEAP_ROOT].expiry > now_ms) {		
		wait_ms = ttl->heap[TTL_HEAP_ROOT].expiry - now_ms;
		ks_log(KS_LOG_INFO, "Waiting %d for TTL expiration of %s", (uint32_t)wait_ms, ks_uuid_thr_str(&ttl->heap[TTL_HEAP_ROOT].id));
	}

	// wait for TTL
	if (wait_ms) {
		ks_cond_timedwait(ttl->cond, wait_ms);
		now_ms = ks_time_now_ms();
	}

	// check for TTL expiration
	if (ttl->heap[TTL_HEAP_ROOT].expiry && ttl->heap[TTL_HEAP_ROOT].expiry <= now_ms) {
		// TTL expired
		*id = ttl->heap[TTL_HEAP_ROOT].id;
		ttl_heap_remove(ttl);
		ks_cond_unlock(ttl->cond);
		return KS_STATUS_SUCCESS;
	}

	// Nothing expired
	ks_cond_unlock(ttl->cond);
	return KS_STATUS_TIMEOUT;
}

static void *ttl_tracker_thread(ks_thread_t *thread, void *data)
{
	swclt_ttl_tracker_t *ttl = (swclt_ttl_tracker_t *)data;
	ks_log(KS_LOG_INFO, "TTL tracker thread running");
	while (ks_thread_stop_requested(thread) == KS_FALSE) {
		ks_uuid_t id = { 0 };
		if (ttl_tracker_next(ttl, &id) == KS_STATUS_SUCCESS) {
			swclt_cmd_t *cmd = NULL;
			ks_hash_read_lock(ttl->conn->outstanding_requests);
			if ((cmd = ks_hash_search(ttl->conn->outstanding_requests, &id, KS_UNLOCKED))) {
				swclt_cmd_report_failure_fmt_m(*cmd, KS_STATUS_TIMEOUT, "TTL expired for command %s", ks_uuid_thr_str(&id));
				ks_log(KS_LOG_INFO, "TTL expired for command %s", ks_uuid_thr_str(&id));
				ks_hash_read_unlock(ttl->conn->outstanding_requests);
			} else {
				ks_hash_read_unlock(ttl->conn->outstanding_requests);
			}
			deregister_cmd(ttl->conn, id);
		}
	}
	ks_log(KS_LOG_INFO, "TTL tracker thread finished");
	return NULL;
}

static void ttl_tracker_destroy(swclt_ttl_tracker_t **ttl)
{
	if (ttl && *ttl) {
		ks_log(KS_LOG_INFO, "Destroying TTL tracker");
		if ((*ttl)->thread && ks_thread_request_stop((*ttl)->thread) != KS_STATUS_SUCCESS) {
			*ttl = NULL;
			ks_log(KS_LOG_ERROR, "Failed to stop TTL thread.  Leaking TTL data and moving on.");
			return;
		}
		ks_cond_lock((*ttl)->cond);
		ks_cond_broadcast((*ttl)->cond);
		ks_cond_unlock((*ttl)->cond);
		ks_thread_destroy(&(*ttl)->thread);
		ks_cond_destroy(&(*ttl)->cond);
		ks_pool_free(ttl);
	}
}

static void ttl_tracker_create(ks_pool_t *pool, swclt_ttl_tracker_t **ttl, swclt_conn_t *ctx)
{
	ks_status_t status;
	*ttl = ks_pool_alloc(pool, sizeof(swclt_ttl_tracker_t));
	ks_cond_create(&(*ttl)->cond, pool);
	(*ttl)->conn = ctx;
	if (status = ks_thread_create(&(*ttl)->thread, ttl_tracker_thread, *ttl, NULL)) {
		ks_abort_fmt("Failed to allocate connection TTL thread: %lu", status);
	}
}

static ks_handle_t *dup_handle(swclt_conn_t *conn, ks_handle_t handle)
{
	ks_handle_t *dup = ks_pool_alloc(conn->pool, sizeof(handle));
	ks_assertd(dup);
	memcpy(dup, &handle, sizeof(handle));
	return dup;
}

static ks_status_t register_cmd(swclt_conn_t *ctx, swclt_cmd_t cmd)
{
	ks_status_t status;
	ks_uuid_t id = { 0 };
	uint32_t ttl_ms = { 0 };

	if (status = swclt_cmd_id(cmd, &id))
		return status;
	if (status = swclt_cmd_ttl(cmd, &ttl_ms))
		return status;

	ks_log(KS_LOG_DEBUG, "Tracking command handle: %16.16llx with id: %s and TTL: %d", cmd, ks_uuid_thr_str(&id), ttl_ms);

	if (!ctx->ttl || (status = ttl_tracker_watch(ctx->ttl, ks_time_now_ms() + (ks_time_t)ttl_ms, id))) {
		ks_log(KS_LOG_ERROR, "Failed to track TTL for command: %16.16llx with id: %s and TTL: %d", cmd, ks_uuid_thr_str(&id), ttl_ms);
		return status;
	}
	return ks_hash_insert(ctx->outstanding_requests, ks_uuid_dup(ctx->pool, &id), dup_handle(ctx, cmd));
}

static void deregister_cmd(swclt_conn_t *ctx, ks_uuid_t id)
{
	ks_hash_remove(ctx->outstanding_requests, &id);
}

static ks_status_t submit_result(swclt_conn_t *ctx, swclt_cmd_t cmd)
{
	SWCLT_CMD_TYPE type;
	ks_status_t status;

	if (!ctx->wss || ctx->wss->failed) {
		return KS_STATUS_FAIL;
	}

	if (status = swclt_cmd_type(cmd, &type)) {
		return status;
	}

	ks_assert(type == SWCLT_CMD_TYPE_RESULT || type == SWCLT_CMD_TYPE_ERROR);

	return swclt_wss_write_cmd(ctx->wss, cmd);
}

static ks_status_t submit_request(swclt_conn_t *ctx, swclt_cmd_t cmd)
{
	ks_status_t status;
	uint32_t flags = 0;

	/* Check state */
	if (!ctx->wss || ctx->wss->failed) {
		return KS_STATUS_FAIL;
	}

	if (status = swclt_cmd_flags(cmd, &flags))
		return status;

	ks_log(KS_LOG_DEBUG, "Submitting request: %s", ks_handle_describe(cmd));

	/* Register this cmd in our outstanding requests if we expect a reply */
	if (!(flags & SWCLT_CMD_FLAG_NOREPLY) && (status = register_cmd(ctx, cmd))) {
		ks_log(KS_LOG_WARNING, "Failed to register cmd: %lu", status);
		return status;
	}

	/* And write it on the socket */
	if (status = swclt_wss_write_cmd(ctx->wss, cmd)) {
		ks_log(KS_LOG_WARNING, "Failed to write to websocket: %lu", status);
		return status;
	}

	/* If the command has a reply, wait for it (it will get
	 * de-registered by wait_cmd_result) */
	if (!(flags & SWCLT_CMD_FLAG_NOREPLY)) {
		swclt_cmd_cb_t cb;
		void *cb_data;

		/* Don't wait if a callback is set */
		if (status = swclt_cmd_cb(cmd, &cb, &cb_data))
			return status;

		if (!cb) {
			if (status = swclt_cmd_wait_result(cmd)) {
				ks_log(KS_LOG_WARNING, "Failed to wait for cmd: %lu", status);
			}
		}
		return status;
	}

	return status;
}

static ks_status_t on_incoming_request(swclt_conn_t *ctx, ks_json_t *payload, swclt_frame_t **frame)
{
	const char *method;
	ks_uuid_t id;
	swclt_cmd_t cmd = KS_NULL_HANDLE;
	ks_status_t status;

	/* Check state */
	if (!ctx->wss || ctx->wss->failed) {
		return KS_STATUS_FAIL;
	}

	ks_log(KS_LOG_DEBUG, "Handling incoming request: %s", (*frame)->data);

	if (!(method = ks_json_get_object_string(payload, "method", NULL))) {
		ks_log(KS_LOG_WARNING, "Invalid response received: %s", (*frame)->data);
		return KS_STATUS_INVALID_ARGUMENT;
	}

	id = ks_uuid_from_str(ks_json_get_object_string(payload, "id", ""));
	if (ks_uuid_is_null(&id)) {
		ks_log(KS_LOG_WARNING, "Response missing id: %s", (*frame)->data);
		return KS_STATUS_INVALID_ARGUMENT;
	}

	if (status = swclt_cmd_create_frame(
			&cmd,
			NULL,
			NULL,
			*frame,
			0,
			BLADE_METHOD_FLAGS(method))) {
		ks_log(KS_LOG_WARNING, "Failed to create command (status: %lu) from frame: %s", status, (*frame)->data);
		return status;
	}

	ks_log(KS_LOG_DEBUG, "Dispatching incoming request method: %s id: %s", method, ks_uuid_thr_str(&id));

	/* And we're in charge of the frame now, we copied it, so free it */
	ks_pool_free(frame);

	/* And raise the client */
	return ctx->incoming_cmd_cb(ctx, cmd, ctx->incoming_cmd_cb_data);
}

static ks_status_t on_incoming_frame(swclt_wss_t *wss, swclt_frame_t **frame, swclt_conn_t *ctx)
{
	ks_json_t *payload = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	const char *method;
	ks_uuid_t id;
	swclt_cmd_t *outstanding_cmd = NULL;
	swclt_cmd_t cmd;
	ks_bool_t async = KS_FALSE;

	ks_log(KS_LOG_DEBUG, "Handling incoming frame: %s", (*frame)->data);

	/* Parse the json out of the frame to figure out what it is */
	if (status = swclt_frame_to_json(*frame, &payload)) {
		ks_log(KS_LOG_ERROR, "Failed to get frame json: %lu", status);
		goto done;
	}

	/* If it's a request, we need to raise this directly with the callback */
	if (ks_json_get_object_item(payload, "params")) {
		/* Ok we don't need this lock anymore since we're not a result */
		status = on_incoming_request(ctx, payload, frame);
		goto done;
	}

	/* Must be a reply, look up our outstanding request */
	id = ks_uuid_from_str(ks_json_get_object_string(payload, "id", ""));
	if (ks_uuid_is_null(&id)) {
		ks_log(KS_LOG_WARNING, "Received invalid payload, missing id: %s", (*frame)->data);
		status = KS_STATUS_INVALID_ARGUMENT;
		goto done;
	}

	ks_hash_read_lock(ctx->outstanding_requests);
	if (!(outstanding_cmd = ks_hash_search(ctx->outstanding_requests, &id, KS_UNLOCKED))) {
		/* Command probably timed out or we don't care about the reply, or a node sent bad data - 
		   this is completely fine and should not cause us to tear down the connection.
		 */
		ks_log(KS_LOG_DEBUG, "Could not locate cmd for frame: %s", (*frame)->data);
		status = KS_STATUS_SUCCESS;
		ks_hash_read_unlock(ctx->outstanding_requests);
		goto done;
	}
	/* Copy the value before we unlock the hash and lose safe access to the value */
	cmd = *outstanding_cmd;
	ks_hash_read_unlock(ctx->outstanding_requests);

	/* Remove the command from outstanding requests */
	deregister_cmd(ctx, id);

	ks_log(KS_LOG_DEBUG, "Fetched cmd handle: %8.8llx", cmd);

	if (status = swclt_cmd_method(cmd, &method)) {
		ks_log(KS_LOG_ERROR, "Failed to get command method: %lu", status);
		goto done;
	}

	/* Great, feed it the reply */
	if (status = swclt_cmd_parse_reply_frame(cmd, *frame, &async)) {
		ks_log(KS_LOG_ERROR, "Failed to parse command reply: %lu", status);
		goto done;
	}

	ks_log(KS_LOG_DEBUG, "Successfully read command result: %s", ks_handle_describe(cmd));

done:

	ks_pool_free(frame);

	if (payload) {
		ks_json_delete(&payload);
	}

	if (async) {
		ks_log(KS_LOG_DEBUG, "Destroying command: %s", ks_handle_describe(cmd));
		ks_handle_destroy(&cmd);
	}

	return status;
}

SWCLT_DECLARE(char *) swclt_conn_describe(swclt_conn_t *ctx)
{
	if (ctx) {
		return ks_psprintf(ctx->pool, "SWCLT Connection to %s:%d - ", ctx->info.wss.address, ctx->info.wss.port);
	}
	return NULL;
}

static ks_status_t do_logical_connect(swclt_conn_t *ctx, ks_uuid_t previous_sessionid, ks_json_t **authentication)
{
	swclt_cmd_t cmd = CREATE_BLADE_CONNECT_CMD(previous_sessionid, authentication);
	SWCLT_CMD_TYPE cmd_type;
	ks_json_t *error = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;

	if (!cmd) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	if (status = submit_request(ctx, cmd))
		goto done;

	if (status = swclt_cmd_type(cmd, &cmd_type)) {
		ks_log(KS_LOG_ERROR, "Unable to get command type");
		goto done;
	}
	if (cmd_type == SWCLT_CMD_TYPE_ERROR) {
		if (status = swclt_cmd_error(cmd, &error)) {
			ks_log(KS_LOG_ERROR, "Unable to get command error");
		}
		goto done;
	}
	
	if (status = swclt_cmd_parse(cmd, ctx->pool,
								  (swclt_cmd_parse_cb_t)BLADE_CONNECT_RPL_PARSE, (void **)&ctx->blade_connect_rpl)) {
		ks_log(KS_LOG_ERROR, "Unable to parse connect reply");
		goto done;
	}

	/* Great snapshot our info types */
	ctx->info.sessionid = ctx->blade_connect_rpl->sessionid;
	ctx->info.nodeid = ks_pstrdup(ctx->pool, ctx->blade_connect_rpl->nodeid);
	ctx->info.master_nodeid = ctx->blade_connect_rpl->master_nodeid;

done:
	/* If the caller wants a call back for connect do that too */
	if (ctx->connect_cb) {
		if (status = ctx->connect_cb(ctx, error, ctx->blade_connect_rpl, ctx->connect_cb_data)) {
			ks_log(KS_LOG_WARNING, "Connect callback returned error: %lu", status);
		}
	}

	ks_handle_destroy(&cmd);

	return status;
}

static void on_wss_failed(swclt_wss_t *wss, void *data)
{
	swclt_conn_t *conn = (swclt_conn_t *)data;
	if (conn->failed_cb) {
		conn->failed_cb(conn, conn->failed_cb_data);
	}
}

static ks_status_t connect_wss(swclt_conn_t *ctx, ks_uuid_t previous_sessionid, ks_json_t **authentication)
{
	ks_status_t status;

	ks_log(KS_LOG_DEBUG, "Initiating websocket connection");

	/* First destroy the previous wss if set */
	if (ctx->wss) {
		if (!ks_uuid_is_null(&ctx->info.sessionid))
			ks_log(KS_LOG_DEBUG, "Destroying previous web socket handle, re-connecting with exiting sessionid: %s", ks_uuid_thr_str(&ctx->info.sessionid));
		else
			ks_log(KS_LOG_DEBUG, "Destroying previous web socket handle, re-connecting with new sessionid");

		swclt_wss_destroy(&ctx->wss);
	}

	if (!ctx->info.wss.port) {
		ks_log(KS_LOG_INFO, "Port not specified, defaulting to 2100");
		ctx->info.wss.port = 2100;
	}
	
	ks_log(KS_LOG_INFO, "Connecting to %s:%d/%s", ctx->info.wss.address, ctx->info.wss.port, ctx->info.wss.path);

	/* Create our websocket transport */
	if (status = swclt_wss_connect(ctx->pool, &ctx->wss,
			(swclt_wss_incoming_frame_cb_t)on_incoming_frame, ctx,
			(swclt_wss_failed_cb_t)on_wss_failed, ctx,
			ctx->info.wss.address, ctx->info.wss.port, ctx->info.wss.path, ctx->info.wss.connect_timeout_ms, ctx->info.wss.ssl))
		return status;

	/* Create TTL tracking thread */
	ttl_tracker_create(ctx->pool, &ctx->ttl, ctx);

	/* Now perform a logical connect to blade with the connect request */
	if (status = do_logical_connect(ctx, previous_sessionid, authentication))
		return status;

	return status;
}

SWCLT_DECLARE(void) swclt_conn_destroy(swclt_conn_t **conn)
{
	if (conn && *conn) {
		swclt_wss_destroy(&(*conn)->wss);
		ks_hash_destroy(&(*conn)->outstanding_requests);
		ttl_tracker_destroy(&(*conn)->ttl);
		ks_pool_free(conn);
	}
}

SWCLT_DECLARE(ks_status_t) swclt_conn_connect_ex(
	ks_pool_t *pool,
	swclt_conn_t **conn,
	swclt_conn_incoming_cmd_cb_t incoming_cmd_cb,
	void *incoming_cmd_cb_data,
	swclt_conn_connect_cb_t connect_cb,
	void *connect_cb_data,
	swclt_conn_failed_cb_t failed_cb,
	void *failed_cb_data,
	swclt_ident_t *ident,
	ks_uuid_t previous_sessionid,
	ks_json_t **authentication,
	const SSL_CTX *ssl)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	swclt_conn_t *new_conn = ks_pool_alloc(pool, sizeof(swclt_conn_t));
	new_conn->pool = pool;

	ks_log(KS_LOG_INFO, "Initiating connection to: %s (parsed port: %u) at /%s", ident->host, (unsigned int)ident->portnum, ident->path);

	new_conn->incoming_cmd_cb = incoming_cmd_cb;
	new_conn->incoming_cmd_cb_data = incoming_cmd_cb_data;
	new_conn->connect_cb = connect_cb;
	new_conn->connect_cb_data = connect_cb_data;
	new_conn->failed_cb = failed_cb;
	new_conn->failed_cb_data = failed_cb_data;

	/* Fill in the info on behalf of wss so we can re-use the same connect api
	 * either during a reconnect or an initial connect */
	strncpy(new_conn->info.wss.address, ident->host, sizeof(new_conn->info.wss.address));
	new_conn->info.wss.port = ident->portnum;
	new_conn->info.wss.ssl = (SSL_CTX *)ssl;
	if (ident->path) strncpy(new_conn->info.wss.path, ident->path, sizeof(new_conn->info.wss.path));
	new_conn->info.wss.connect_timeout_ms = 10000;

	/* Create our request hash */
	if (status = ks_hash_create(&new_conn->outstanding_requests, KS_HASH_MODE_UUID,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_FREE_KEY, new_conn->pool))
		goto done;

	/* Connect our websocket */
	if (status = connect_wss(new_conn, previous_sessionid, authentication))
		goto done;

done:
	if (status != KS_STATUS_SUCCESS) {
		swclt_conn_destroy(&new_conn);
	}
	*conn = new_conn;
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_conn_connect(
	ks_pool_t *pool,
	swclt_conn_t **conn,
	swclt_conn_incoming_cmd_cb_t incoming_cmd_cb,
	void *incoming_cmd_cb_data,
	swclt_ident_t *ident,
	ks_json_t **authentication,
	const SSL_CTX *ssl)
{
	return swclt_conn_connect_ex(
		pool,
		conn,
		incoming_cmd_cb,
		incoming_cmd_cb_data,
		NULL,
		NULL,
		NULL,
		NULL,
		ident,
		ks_uuid_null(),
		authentication,
		ssl);
}

SWCLT_DECLARE(ks_status_t) swclt_conn_submit_result(swclt_conn_t *conn, swclt_cmd_t cmd)
{
	if (conn) {
		return submit_result(conn, cmd);
	}
	return KS_STATUS_FAIL;
}

SWCLT_DECLARE(ks_status_t) swclt_conn_submit_request(swclt_conn_t *conn, swclt_cmd_t cmd)
{
	if (conn) {
		return submit_request(conn, cmd);
	}
	return KS_STATUS_FAIL;
}

/* Private due to un-implemented caller ownership semantics, internal use only */
ks_status_t swclt_conn_info(swclt_conn_t *conn, swclt_conn_info_t *info)
{
	memcpy(info, &conn->info, sizeof(conn->info));
	return KS_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
