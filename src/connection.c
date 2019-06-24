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
#include "signalwire-client-c/internal/connection.h"

#define ks_time_now_ms() ks_time_ms(ks_time_now())

static ks_handle_t * __dupe_handle(swclt_conn_ctx_t *ctx, ks_handle_t handle)
{
	ks_handle_t *dup = ks_pool_alloc(ctx->base.pool, sizeof(handle));
	ks_assertd(dup);
	memcpy(dup, &handle, sizeof(handle));

	ks_log(KS_LOG_DEBUG, "Duplicated handle: %16.16llx", handle);
	return dup;
}

static ks_status_t __register_cmd(swclt_conn_ctx_t *ctx, swclt_cmd_t cmd, ks_uuid_t *id, uint32_t *flags, uint32_t *ttl_ms)
{
	ks_status_t status;

	if (status = swclt_cmd_id(cmd, id))
		return status;
	if (status = swclt_cmd_flags(cmd, flags))
		return status;
	if (status = swclt_cmd_ttl(cmd, ttl_ms))
		return status;

	/* Set this handle as a child of ours so we can free it if needed */
	ks_handle_set_parent(cmd, ctx->base.handle);

	ks_log(KS_LOG_DEBUG, "Inserting command handle: %16.16llx into hash for command key: %s", cmd, ks_uuid_thr_str(id));

	return ks_hash_insert(ctx->outstanding_requests, ks_uuid_dup(ctx->base.pool, id), __dupe_handle(ctx, cmd));
}

static void __context_deinit(
	swclt_conn_ctx_t *ctx)
{
	ks_handle_destroy(&ctx->wss);
	ks_hash_destroy(&ctx->outstanding_requests);
}

static ks_status_t __wait_cmd_result(swclt_conn_ctx_t *ctx, swclt_cmd_t cmd, SWCLT_CMD_TYPE *type)
{
	uint32_t ttl_ms, ttl_remaining_ms, duration_total_ms;
	ks_time_t start_sec;
	const char *method;
	ks_status_t status;

	if (status = swclt_cmd_ttl(cmd, &ttl_ms))
		return status;
	if (status = swclt_cmd_method(cmd, &method))
		return status;
	ttl_remaining_ms = ttl_ms;
	ks_assert(!(ttl_ms != 0 && ttl_ms < 1000));

	/* Now wait for it to get completed */
	start_sec = ks_time_now_sec();
	ks_cond_lock(ctx->cmd_condition);
	while (KS_TRUE) {
		if (status = swclt_cmd_type(cmd, type))
			break;

		switch (*type) {
			case SWCLT_CMD_TYPE_ERROR:
			case SWCLT_CMD_TYPE_RESULT:
				goto done;

			case SWCLT_CMD_TYPE_REQUEST:
				break;

			case SWCLT_CMD_TYPE_FAILURE:
				ks_log(KS_LOG_WARNING, "Command failure", status);
				goto done;
			default:
				ks_abort_fmt("Invalid command type: %lu", *type);
		}

		if (ttl_ms) {
			if (status = ks_cond_timedwait(ctx->cmd_condition, ttl_remaining_ms)) {
				if (status != KS_STATUS_TIMEOUT) {
					ks_log(KS_LOG_WARNING, "Condition wait failed: %lu", status);
					break;
				}
			}

			/* Update remaining and figure out if we've timed out */
			duration_total_ms = (uint32_t)((ks_time_now_sec() - start_sec) * 1000);
			if (duration_total_ms > ttl_remaining_ms) {
				swclt_cmd_report_failure_fmt_m(cmd, KS_STATUS_TIMEOUT, "TTL expired for command: %s (ttl_ms: %lu)", method, ttl_ms);

				/* Return success, error is really within the cmd */
				status = KS_STATUS_SUCCESS;
				break;
			}

			ttl_remaining_ms -= duration_total_ms;
		}
		else
			ks_cond_wait(ctx->cmd_condition);
	}

done:
	ks_cond_unlock(ctx->cmd_condition);

	return status;
}

static void __deregister_cmd(swclt_conn_ctx_t *ctx, swclt_cmd_t cmd, ks_uuid_t id)
{
	ks_hash_remove(ctx->outstanding_requests, &id);
}

static ks_status_t __wait_outstanding_cmd_result(swclt_conn_ctx_t *ctx, swclt_cmd_t cmd, SWCLT_CMD_TYPE *type)
{
	ks_uuid_t id;
	ks_status_t status;

	if (status = swclt_cmd_id(cmd, &id))
		return status;

	ks_hash_read_lock(ctx->outstanding_requests);
	if (!ks_hash_search(ctx->outstanding_requests, &id, KS_UNLOCKED)) {
		ks_log(KS_LOG_WARNING, "Failed to lookup command: %16.16llx", cmd);
		ks_hash_read_unlock(ctx->outstanding_requests);
		return KS_STATUS_FAIL;
	}
    ks_hash_read_unlock(ctx->outstanding_requests);

	status = __wait_cmd_result(ctx, cmd, type);

	/* Great, remove it */
	__deregister_cmd(ctx, cmd, id);

	return status;
}

static ks_status_t __submit_result(swclt_conn_ctx_t *ctx, swclt_cmd_t cmd)
{
	SWCLT_CMD_TYPE type;
	ks_status_t status;

	if (status = swclt_hstate_check_ctx(&ctx->base, "Submit result denied due to state"))
		return status;

	if (status = swclt_cmd_type(cmd, &type))
		return status;

	ks_assert(type == SWCLT_CMD_TYPE_RESULT || type == SWCLT_CMD_TYPE_ERROR);

	return swclt_wss_write_cmd(ctx->wss, cmd);
}

static ks_status_t __submit_request(swclt_conn_ctx_t *ctx, swclt_cmd_t cmd)
{
	uint32_t flags;
	ks_uuid_t id;
	SWCLT_CMD_TYPE type;
	uint32_t ttl_ms;
	ks_status_t status;

	/* Check state */
	if (status = swclt_hstate_check_ctx(&ctx->base, "Submit request denied due to state"))
		return status;

	ks_log(KS_LOG_DEBUG, "Submitting request: %s", ks_handle_describe(cmd));

	/* Register this cmd in our outstanding requests */
	if (status = __register_cmd(ctx, cmd, &id, &flags, &ttl_ms)) {
		ks_log(KS_LOG_WARNING, "Failed to register cmd: %lu", status);
		return status;
	}

	/* And write it on the socket */
	if (status = swclt_wss_write_cmd(ctx->wss, cmd)) {
		ks_log(KS_LOG_WARNING, "Failed to write to websocket: %lu", status);
		return status;
	}

	/* Now mark this command as finally submitted, this will atomically now
	 * allow this command to be servied by the session thread */
	if (status = swclt_cmd_set_submit_time(cmd, ks_time_now())) {
		ks_log(KS_LOG_CRIT, "Failed to update commands submit time: %lu", status);
		return status;
	}

	ks_log(KS_LOG_DEBUG, "Requesting service for command ttl of: %lums", ttl_ms);

	/* Now ask to be serviced by its ttl time */
	swclt_hmgr_request_service_in(&ctx->base, ttl_ms);

	/* If the command has a reply, wait for it (it will get
	 * de-registered by __wait_outstanding_cmd_result) */
	if (!(flags & SWCLT_CMD_FLAG_NOREPLY)) {
		swclt_cmd_cb_t cb;
		void *cb_data;

		/* Don't wait if a callback is set */
		if (status = swclt_cmd_cb(cmd, &cb, &cb_data))
			return status;

		if (!cb) {
			if (status = __wait_outstanding_cmd_result(ctx, cmd, &type)) {
				ks_log(KS_LOG_WARNING, "Failed to wait for cmd: %lu", status);
			}
		}
		return status;
	}

	/* No reply so de-register it */
	__deregister_cmd(ctx, cmd, id);
	return status;
}

static ks_status_t __on_incoming_request(swclt_conn_ctx_t *ctx, ks_json_t *payload, swclt_frame_t frame)
{
	const char *method;
	ks_uuid_t id;
	swclt_cmd_t cmd = KS_NULL_HANDLE;
	ks_status_t status;

	/* Check state */
	if (status = swclt_hstate_check_ctx(&ctx->base, "Ignoring incoming request due to state:"))
		return status;

	ks_log(KS_LOG_DEBUG, "Handling incoming request: %s", ks_handle_describe(frame));

	if (!(method = ks_json_get_object_cstr_def(payload, "method", NULL))) {
		ks_log(KS_LOG_WARNING, "Invalid response received: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	id = ks_json_get_object_uuid(payload, "id");
	if (ks_uuid_is_null(&id)) {
		ks_log(KS_LOG_WARNING, "Response missing id: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	if (status = swclt_cmd_create_frame(
			&cmd,
			NULL,
			NULL,
			frame,
			0,
			BLADE_METHOD_FLAGS(method))) {
		ks_log(KS_LOG_WARNING, "Failed to create command (status: %lu) from frame: %s", status, ks_handle_describe(frame));
		return status;
	}

	ks_log(KS_LOG_DEBUG, "Dispatching incoming request method: %s id: %s", method, ks_uuid_thr_str(&id));

	/* Bind this cmd to our connection in case the callback does not free it */
	ks_handle_set_parent(cmd, ctx->base.handle);

	/* And we're in charge of the frame now, we copied it, so invalidae it */
	ks_handle_destroy(&frame);

	/* And raise the client */
	return ctx->incoming_cmd_cb(ctx->base.handle, cmd, ctx->incoming_cmd_cb_data);
}

static ks_status_t __on_incoming_frame(swclt_wss_t wss, swclt_frame_t frame, swclt_conn_ctx_t *ctx)
{
	ks_json_t *payload;
	ks_status_t status;
	const char *method;
	ks_uuid_t id;
	swclt_cmd_t *outstanding_cmd = NULL;
	swclt_cmd_t cmd;
	ks_bool_t async = KS_FALSE;

	ks_log(KS_LOG_DEBUG, "Handling incoming frame: %s", ks_handle_describe(frame));

	/* Lock to synchronize with the waiter thread */
	ks_cond_lock(ctx->cmd_condition);

	/* Parse the json out of the frame to figure out what it is */
	if (status = swclt_frame_get_json(frame, &payload)) {
		ks_log(KS_LOG_ERROR, "Failed to get frame json: %lu", status);
		goto done;
	}

	/* If its a request, we need to raise this directly with the callback */
	if (ks_json_get_object_item(payload, "params")) {
		/* Ok we don't need this lock anymore sinc we're not a result */
		ks_cond_unlock(ctx->cmd_condition);
		return __on_incoming_request(ctx, payload, frame);
	}

	/* Must be a reply, look up our outstanding request */
	id = ks_json_get_object_uuid(payload, "id");
	if (ks_uuid_is_null(&id)) {
		ks_log(KS_LOG_WARNING, "Received invalid payload, missing id: %s", ks_handle_describe(frame));
		status = KS_STATUS_INVALID_ARGUMENT;
		goto done;
	}

	ks_hash_read_lock(ctx->outstanding_requests);
	if (!(outstanding_cmd = ks_hash_search(ctx->outstanding_requests, &id, KS_UNLOCKED))) {

		/* Command probably timed out */
		ks_log(KS_LOG_DEBUG, "Could not locate cmd for frame: %s", ks_handle_describe(frame));

		/* Unexpected, break in case it happens in a debugger */
		status = KS_STATUS_INVALID_ARGUMENT;
		ks_hash_read_unlock(ctx->outstanding_requests);
		goto done;
	}
	ks_hash_read_unlock(ctx->outstanding_requests);

	/* Copy the handle out before we remove the memory storing it in the hash */
	cmd = *outstanding_cmd;
	
	/* Remove the command from outstanding requests */
	__deregister_cmd(ctx, *outstanding_cmd, id);

	/* Right away clear this commands ttl to prevent a timeout during dispatch */
	if (status = swclt_cmd_set_submit_time(cmd, 0)) {
		ks_log(KS_LOG_ERROR, "Failed to set ttl to 0 on command while processing result: %s", ks_handle_describe(cmd));
		goto done;
	}

	ks_log(KS_LOG_DEBUG, "Fetched cmd handle: %8.8llx", cmd);

	if (status = swclt_cmd_method(cmd, &method)) {
		ks_log(KS_LOG_ERROR, "Failed to get command method: %lu", status);
		goto done;
	}

	/* Great, feed it the reply */
	if (status = swclt_cmd_parse_reply_frame(cmd, frame, &async)) {
		ks_log(KS_LOG_ERROR, "Failed to parse command reply: %lu", status);
		goto done;
	}

	ks_log(KS_LOG_DEBUG, "Successfully read command result: %s", ks_handle_describe(cmd));

	/* Now raise the signal */
	ks_cond_broadcast(ctx->cmd_condition);

done:
	ks_cond_unlock(ctx->cmd_condition);

	ks_handle_destroy(&frame);

	if (async) {
		ks_log(KS_LOG_DEBUG, "Destroying command: %s", ks_handle_describe(cmd));
		ks_handle_destroy(&cmd);
	}

	return status;
}

static void __context_describe(swclt_conn_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	/* We have to do all this garbage because of the poor decision to nest ks_handle_describe() calls that return a common thread local buffer */
	const char *desc = ks_handle_describe(ctx->wss);
	ks_size_t desc_len = strlen(desc);
	char preamble_buf[256] = { 0 };
	ks_size_t preamble_len = 0;
	snprintf(preamble_buf, sizeof(preamble_buf), "SWCLT Connection to %s:%d - ", ctx->info.wss.address, ctx->info.wss.port);
	preamble_len = strlen(preamble_buf);
	if (desc_len + preamble_len + 1 > buffer_len) {
		desc_len = buffer_len - preamble_len - 1;
	}
	memmove(buffer + preamble_len, desc, desc_len + 1);
	memcpy(buffer, preamble_buf, preamble_len);
	buffer[buffer_len - 1] = '\0';
}

static ks_status_t __do_logical_connect(swclt_conn_ctx_t *ctx, ks_uuid_t previous_sessionid, ks_json_t **authentication)
{
	swclt_cmd_t cmd = CREATE_BLADE_CONNECT_CMD(previous_sessionid, authentication);
	SWCLT_CMD_TYPE cmd_type;
	ks_json_t *error = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;

	if (!cmd) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	if (status = __submit_request(ctx, cmd))
		goto done;

	if (status = swclt_cmd_type(cmd, &cmd_type)) {
		ks_log(KS_LOG_ERROR, "Unable to get command type");
		goto done;
	}
	if (cmd_type == SWCLT_CMD_TYPE_ERROR) {
		if (status = swclt_cmd_error(cmd, (const ks_json_t **)&error)) {
			ks_log(KS_LOG_ERROR, "Unable to get command error");
		}
		goto done;
	}
	
	if (status = swclt_cmd_lookup_parse_s(cmd, ctx->base.pool,
										  (swclt_cmd_parse_cb_t)BLADE_CONNECT_RPL_PARSE, (void **)&ctx->blade_connect_rpl)) {
		ks_log(KS_LOG_ERROR, "Unable to parse connect reply");
		goto done;
	}

	/* Great snapshot our info types */
	ctx->info.sessionid = ctx->blade_connect_rpl->sessionid;
	ctx->info.nodeid = ks_pstrdup(ctx->base.pool, ctx->blade_connect_rpl->nodeid);
	ctx->info.master_nodeid = ctx->blade_connect_rpl->master_nodeid;

done:
	/* If the caller wants a call back for connect do that too */
	if (ctx->connect_cb) {
		if (status = ctx->connect_cb(ctx->base.handle, error, ctx->blade_connect_rpl, ctx->connect_cb_data)) {
			ks_log(KS_LOG_WARNING, "Connect callback returned error: %lu", status);
		}
	}

	ks_handle_destroy(&cmd);

	return status;
}

static void __context_service(swclt_conn_ctx_t *ctx)
{
	/* Iterate our outstanding commands, and time out any that haven't received
	 * a response by their ttl amount */
	ks_hash_iterator_t *itt;

	ks_log(KS_LOG_DEBUG, "Checking outstanding commands for ttl timeout: %s", ks_handle_describe_ctx(ctx));

	ks_hash_write_lock(ctx->outstanding_requests);
	for (itt = ks_hash_first(ctx->outstanding_requests, KS_UNLOCKED); itt; ) {
		swclt_cmd_t *_cmd;
		swclt_cmd_t cmd;
		ks_uuid_t *id_key;
		uint32_t ttl_ms;
		ks_time_t cmd_submit_time = 0;
		ks_bool_t remove = KS_FALSE;
		SWCLT_CMD_TYPE type;
		ks_time_t total_time_waited_ms = 0;

		ks_hash_this(itt, (const void **)&id_key, NULL, (void **)&_cmd);

		/* Copy it and let the hash delete it later */
		cmd = *_cmd;

		ks_log(KS_LOG_DEBUG, "Checking command: %s for ttl expiration", ks_handle_describe(cmd));

		/* Now check its time */
		if (swclt_cmd_ttl(cmd, &ttl_ms) || swclt_cmd_submit_time(cmd, &cmd_submit_time) || swclt_cmd_type(cmd, &type)) {
			ks_log(KS_LOG_WARNING, "Removing invalid cmd with id: %s from outstanding requests with handle value: %16.16llx", ks_uuid_thr_str(id_key), cmd);
			remove = KS_TRUE;
		}

		/* The command submit time is is left zero until the command actually is copletely written over the websocket
		 * so don't time these out */
		if (cmd_submit_time == 0)
			goto next;

		/* Check for time travelers */
		if (ks_time_now() < cmd_submit_time) {
			ks_debug_break();
			ks_log(KS_LOG_WARNING, "Invalid time detected in command, removing: %s", ks_handle_describe(cmd));
			remove = KS_TRUE;
		}

		/* Leave commands with inifnite timeouts alone */
		if (ttl_ms == 0)
			goto next;

		if (!remove && type == SWCLT_CMD_TYPE_REQUEST) {
			total_time_waited_ms = ks_time_ms(ks_time_now() - cmd_submit_time);

			if (total_time_waited_ms > ttl_ms) {
				ks_log(KS_LOG_WARNING, "Removing timed out cmd %s (current wait time is: %lums, ttl was: %lums)", ks_handle_describe(cmd), total_time_waited_ms, ttl_ms);
				remove = KS_TRUE;
			} else {
				ks_log(KS_LOG_DEBUG, "Not removing cmd: %s, ttl has not crossed wait time of: %lums (current wait time is: %lums)", ks_handle_describe(cmd), ttl_ms, total_time_waited_ms);

				/* Enqueue a manager service for the remaining amount */
				swclt_hmgr_request_service_in(&ctx->base, ttl_ms - total_time_waited_ms);
			}
		}

	next:

		itt = ks_hash_next(&itt);

		if (remove) {
			ks_log(KS_LOG_DEBUG, "Removing command registered for id: %s", ks_uuid_thr_str(id_key));

			ks_hash_remove(ctx->outstanding_requests, id_key);

			/* Flag it failed with timeout */
			swclt_cmd_report_failure_fmt_m(cmd, KS_STATUS_TIMEOUT, "Command waited: %lums (ttl: %lums)", total_time_waited_ms, ttl_ms);
		}
	}
	ks_hash_write_unlock(ctx->outstanding_requests);
}

static void __on_wss_state_change(swclt_conn_ctx_t *ctx, swclt_hstate_change_t *state_change)
{
	/* When websocket dies, all commands get marked as failed and removed from the outstanding commands hash */
	if (state_change->new_state == SWCLT_HSTATE_DEGRADED) {

		/* Enqueue a state change on ourselves as well */
		swclt_hstate_changed(&ctx->base, SWCLT_HSTATE_DEGRADED,
			 KS_STATUS_FAIL, "Websocket failed");

		ks_log(KS_LOG_WARNING, "Websocket got disconnected and has initiated a state change: %s", swclt_hstate_describe_change(state_change));
		ks_log(KS_LOG_WARNING, "Expiring all outstanding commands");

		ks_hash_iterator_t *itt;

		ks_hash_write_lock(ctx->outstanding_requests);
		for (itt = ks_hash_first(ctx->outstanding_requests, KS_UNLOCKED); itt; ) {
			swclt_cmd_t *_cmd;
			swclt_cmd_t cmd;
			ks_uuid_t *id_key;

			ks_hash_this(itt, (const void **)&id_key, NULL, (void **)&_cmd);

			/* Copy it and let the hash delete it later */
			cmd = *_cmd;

			itt = ks_hash_next(&itt);

			ks_hash_remove(ctx->outstanding_requests, id_key);

			/* Flag it failed with timeout */
			swclt_cmd_report_failure_fmt_s(cmd, state_change->status,
				swclt_hstate_describe_change(state_change));
		}
		ks_hash_write_unlock(ctx->outstanding_requests);
	}
}

static ks_status_t __connect_wss(swclt_conn_ctx_t *ctx, ks_uuid_t previous_sessionid, ks_json_t **authentication)
{
	ks_status_t status;

	ks_log(KS_LOG_DEBUG, "Initiating websocket connection");

	/* First destroy the previous wss if set */
	if (ctx->wss) {
		if (!ks_uuid_is_null(&ctx->info.sessionid))
			ks_log(KS_LOG_DEBUG, "Destroying previous web socket handle, re-connecting with exiting sessionid: %s", ks_uuid_thr_str(&ctx->info.sessionid));
		else
			ks_log(KS_LOG_DEBUG, "Destroying previous web socket handle, re-connecting with new sessionid");

		ks_handle_destroy(&ctx->wss);
	}

	if (!ctx->info.wss.port) {
		ks_log(KS_LOG_INFO, "Port not specified, defaulting to 2100");
		ctx->info.wss.port = 2100;
	}
	
	ks_log(KS_LOG_INFO, "Connecting to %s:%d/%s", ctx->info.wss.address, ctx->info.wss.port, ctx->info.wss.path);

	/* Create our websocket transport */
	if (status = swclt_wss_connect(&ctx->wss, (swclt_wss_incoming_frame_cb_t)__on_incoming_frame,
								   ctx, ctx->info.wss.address, ctx->info.wss.port, ctx->info.wss.path, ctx->info.wss.connect_timeout_ms, ctx->info.wss.ssl))
		return status;

	/* Listen for state changes on the websocket */
	if (status = swclt_hstate_register_listener(&ctx->base, __on_wss_state_change, ctx->wss))
		return status;

	/* Now perform a logical connect to blade with the connect request */
	if (status = __do_logical_connect(ctx, previous_sessionid, authentication))
		return status;

	return status;
}

static ks_status_t __context_init(
	swclt_conn_ctx_t *ctx,
	swclt_conn_incoming_cmd_cb_t incoming_cmd_cb,
	void *incoming_cmd_cb_data,
	swclt_conn_connect_cb_t connect_cb,
	void *connect_cb_data,
	swclt_ident_t *ident,
	ks_uuid_t previous_sessionid,
	ks_json_t **authentication,
	const SSL_CTX *ssl)
{
	ks_status_t status;

	ks_log(KS_LOG_INFO, "Initiating connection to: %s (parsed port: %u) at /%s", ident->host, (unsigned int)ident->portnum, ident->path);

	ctx->incoming_cmd_cb = incoming_cmd_cb;
	ctx->incoming_cmd_cb_data = incoming_cmd_cb_data;
	ctx->connect_cb = connect_cb;
	ctx->connect_cb_data = connect_cb_data;

	/* Fill in the info on behalf of wss so we can re-use the same connect api
	 * either during a reconnect or an initial connect */
	strncpy(ctx->info.wss.address, ident->host, sizeof(ctx->info.wss.address));
	ctx->info.wss.port = ident->portnum;
	ctx->info.wss.ssl = (SSL_CTX *)ssl;
	if (ident->path) strncpy(ctx->info.wss.path, ident->path, sizeof(ctx->info.wss.path));
	ctx->info.wss.connect_timeout_ms = 10000;

	if (status = ks_cond_create(&ctx->cmd_condition, ctx->base.pool))
		goto done;

	/* Create our request hash */
	if (status = ks_hash_create(&ctx->outstanding_requests, KS_HASH_MODE_UUID,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_FREE_KEY, ctx->base.pool))
		goto done;

	/* Enable servicing */
	swclt_hstate_register_service(&ctx->base, __context_service);

	/* Connect our websocket */
	if (status = __connect_wss(ctx, previous_sessionid, authentication))
		goto done;

done:
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_conn_connect_ex(
	swclt_conn_t *conn,
	swclt_conn_incoming_cmd_cb_t incoming_cmd_cb,
	void *incoming_cmd_cb_data,
	swclt_conn_connect_cb_t connect_cb,
	void *connect_cb_data,
	swclt_ident_t *ident,
	ks_uuid_t previous_sessionid,
	ks_json_t **authentication,
	const SSL_CTX *ssl)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M(
		NULL,
		SWCLT_HTYPE_CONN,
		conn,
		swclt_conn_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		incoming_cmd_cb,
		incoming_cmd_cb_data,
		connect_cb,
		connect_cb_data,
		ident,
		previous_sessionid,
		authentication,
		ssl);
}

SWCLT_DECLARE(ks_status_t) swclt_conn_connect(
	swclt_conn_t *conn,
	swclt_conn_incoming_cmd_cb_t incoming_cmd_cb,
	void *incoming_cmd_cb_data,
	swclt_ident_t *ident,
	ks_json_t **authentication,
	const SSL_CTX *ssl)
{
	return swclt_conn_connect_ex(
		conn,
		incoming_cmd_cb,
		incoming_cmd_cb_data,
		NULL,
		NULL,
		ident,
		ks_uuid_null(),
		authentication,
		ssl);
}

SWCLT_DECLARE(ks_status_t) swclt_conn_submit_result(swclt_conn_t conn, swclt_cmd_t cmd)
{
	SWCLT_CONN_SCOPE_BEG(conn, ctx, status);

	status = __submit_result(ctx, cmd);

	SWCLT_CONN_SCOPE_END(conn, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_conn_submit_request(swclt_conn_t conn, swclt_cmd_t cmd)
{
	SWCLT_CONN_SCOPE_BEG(conn, ctx, status);

	status = __submit_request(ctx, cmd);

	SWCLT_CONN_SCOPE_END(conn, ctx, status);
}

/* Private due to un-implemented caller ownership semantics, internal use only */
ks_status_t swclt_conn_info(swclt_conn_t conn, swclt_conn_info_t *info)
{
	SWCLT_CONN_SCOPE_BEG(conn, ctx, status);
	memcpy(info, &ctx->info, sizeof(ctx->info));
	SWCLT_CONN_SCOPE_END(conn, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_conn_get_rates(swclt_conn_t conn, ks_throughput_t *recv, ks_throughput_t *send)
{
	SWCLT_CONN_SCOPE_BEG(conn, ctx, status);
	status = swclt_wss_get_rates(ctx->wss, recv, send);
	SWCLT_CONN_SCOPE_END(conn, ctx, status);
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
