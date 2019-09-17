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
#include "signalwire-client-c/internal/config.h"
#include "signalwire-client-c/internal/session.h"
#include "signalwire-client-c/internal/command.h"
#include "signalwire-client-c/internal/subscription.h"

typedef struct swclt_metric_reg_s {
	int interval;
	ks_time_t timeout;
	int rank;
	ks_bool_t dirty;
} swclt_metric_reg_t;


static ks_status_t __context_state_transition(swclt_sess_ctx_t *ctx, SWCLT_HSTATE new_state);

static void __context_deinit(
	swclt_sess_ctx_t *ctx)
{
	ks_handle_destroy(&ctx->conn);
	ks_hash_destroy(&ctx->subscriptions);
	ks_hash_destroy(&ctx->methods);
	ks_hash_destroy(&ctx->setups);
	ks_hash_destroy(&ctx->metrics);
	swclt_ssl_destroy_context(&ctx->ssl);
	swclt_ident_destroy(&ctx->ident);
}

static const char * __make_subscription_key(swclt_sess_ctx_t *ctx, const char * protocol, const char * channel)
{
	return ks_psprintf(ctx->base.pool, "%s:%s", protocol, channel);
}

static const char * __make_pmethod_key(swclt_sess_ctx_t *ctx, const char *protocol, const char *method)
{
	return ks_psprintf(ctx->base.pool, "%s:%s", protocol, method);
}

static swclt_pmethod_ctx_t * __make_pmethod_value(swclt_sess_ctx_t *ctx, swclt_pmethod_cb_t pmethod, void *cb_data)
{
	swclt_pmethod_ctx_t *pmethod_ctx = ks_pool_alloc(ctx->base.pool, sizeof(swclt_pmethod_ctx_t));
	pmethod_ctx->cb = pmethod;
	pmethod_ctx->cb_data = cb_data;
	return pmethod_ctx;
}

static ks_status_t __setup_ssl(swclt_sess_ctx_t *ctx)
{
	swclt_ssl_destroy_context(&ctx->ssl);

	return swclt_ssl_create_context(ctx->config->private_key_path, ctx->config->client_cert_path, ctx->config->cert_chain_path, &ctx->ssl);
}

static ks_bool_t __session_check_connected(swclt_sess_ctx_t *ctx, ks_bool_t leave_locked_on_connected)
{
	ks_bool_t connected = KS_FALSE;
	SWCLT_HSTATE state;

	/* Lock while we touch the internals of the hstate system */
	ks_spinlock_acquire(&ctx->base.lock);

	state = ctx->base.state;
	if (ctx->base.pending_state_change_service != SWCLT_HSTATE_INVALID)
		state = SWCLT_HSTATE_INVALID;

	connected = state == SWCLT_HSTATE_ONLINE;

	if (!connected || !leave_locked_on_connected)
		ks_spinlock_release(&ctx->base.lock);

	return connected;
}

static ks_status_t __execute_pmethod_cb(
	swclt_sess_ctx_t *ctx,
	const swclt_cmd_ctx_t *cmd_ctx,
   	ks_pool_t *pool,
   	swclt_pmethod_ctx_t *pmethod_ctx,
   	const blade_execute_rqu_t *rqu)
{
	const char *err_message = NULL;
	int err_code = 0;

	/* If the context could not be found, set the response appropriately */
	if (!pmethod_ctx) {
		err_message = ks_psprintf(
					pool,
			   		"Failed to lookup any registered protocol method handlers for protocol: %s command: %s",
				   	rqu->protocol,
				   	ks_handle_describe_ctx(cmd_ctx));
		err_code = -32601;
	} else {
		/* Raise the callback, if they respond with a general failure fail for them */
		ks_status_t cb_result;

		ks_log(KS_LOG_INFO, "Initiating execute for protocol: %s", rqu->protocol);

		if (cb_result = pmethod_ctx->cb(ctx->base.handle, cmd_ctx->base.handle, rqu, pmethod_ctx->cb_data)) {
			err_message = ks_psprintf(
						pool,
					   	"Protocol method callback returned status: (%lu) command: %s",
					   	cb_result,
					   	ks_handle_describe_ctx(cmd_ctx));
			err_code = -32603;
		}

		ks_log(KS_LOG_INFO, "Completed execute for protocol: %s (%lu)", rqu->protocol, cb_result);
	}

	/* Now verify the command was properly setup for result processing */
	if (cmd_ctx->type != SWCLT_CMD_TYPE_RESULT && cmd_ctx->type != SWCLT_CMD_TYPE_ERROR) {
		ks_debug_break();

		/* Force one for them */
		err_message = ks_psprintf(
					pool,
					"Protocol method failed to set result in command: %s",
					ks_handle_describe_ctx(cmd_ctx));
		err_code = -32607;
	}

	/* If we errored, package up an error response */
	if (err_code) {
		ks_status_t status;

		ks_log(KS_LOG_ERROR, err_message);

		ks_json_t *err = BLADE_EXECUTE_ERR_MARSHAL(
					cmd_ctx->base.pool,
					&(blade_execute_err_t){
						rqu->requester_nodeid,
						rqu->responder_nodeid,
						err_code,
						err_message});

		if (status = swclt_cmd_set_error(cmd_ctx->base.handle, &err)) {
			ks_log(KS_LOG_ERROR, "Failed to set result in command: %lu", status);
			ks_json_delete(&err);
			ks_pool_free(&err_message);
		}

		return status;
	}

	// All is well in the world
	return KS_STATUS_SUCCESS;
}

static ks_status_t __on_incoming_cmd(swclt_conn_t conn, swclt_cmd_t cmd, swclt_sess_ctx_t *ctx)
{
	const char * method;
	ks_status_t status;
	ks_json_t *request;
	ks_pool_t *cmd_pool;
	const swclt_cmd_ctx_t *cmd_ctx;

	/* Reserve the cmd while we mess with it */
	if (status = swclt_cmd_ctx_get(cmd, &cmd_ctx)) {
		ks_log(KS_LOG_ERROR, "Failed to reserve command for pmethod callback");
		return status;
	}

	ks_log(KS_LOG_DEBUG, "Handling incoming command: %s from transport", ks_handle_describe_ctx(cmd_ctx));

	/* Retain thread saftey when messing with cmd internals */
	swclt_cmd_ctx_lock(cmd_ctx);

	/* Use the commands pool for all allocations */
	cmd_pool = (ks_pool_t *)cmd_ctx->base.pool;

	/* Grab information about the method */
	method = cmd_ctx->method;
	request = cmd_ctx->request;

	/* Keep it locked until we parse the request */
	if (!strcmp(method, BLADE_BROADCAST_METHOD)) {
		/* Locate the protocol */
		blade_broadcast_rqu_t *rqu;
		swclt_sub_t *sub;
		const char *key;

		status = BLADE_BROADCAST_RQU_PARSE(cmd_pool, request, &rqu);
		swclt_cmd_ctx_unlock(cmd_ctx);

		if (status) {
			ks_log(KS_LOG_ERROR, "Failed to parse broadcast command: %s (%lu)",
				ks_handle_describe_ctx(cmd_ctx), status);
			goto done;
		}

		key = __make_subscription_key(ctx, rqu->protocol, rqu->channel);
		sub = ks_hash_search(ctx->subscriptions, key, KS_UNLOCKED);
		ks_pool_free(&key);

		if (!sub) {
			ks_log(KS_LOG_ERROR, "Could not locate sub for protocol: %s channel: %s command: %s",
				rqu->protocol, rqu->channel, ks_handle_describe_ctx(cmd_ctx));
			BLADE_BROADCAST_RQU_DESTROY(&rqu);
			status = KS_STATUS_NOT_FOUND;
			goto done;
		}

		status = swclt_sub_invoke(*sub, ctx->base.handle, rqu);

		BLADE_BROADCAST_RQU_DESTROY(&rqu);
		goto done;
	} else if (!strcmp(method, BLADE_DISCONNECT_METHOD)) {
		//blade_disconnect_rqu_t *rqu;

		//status = BLADE_DISCONNECT_RQU_PARSE(cmd_pool, request, &rqu);

		swclt_cmd_ctx_unlock(cmd_ctx);

		//if (status) {
		//	ks_log(KS_LOG_ERROR, "Failed to parse netcast command: %s (%lu)", ks_handle_describe_ctx(cmd_ctx), status);
		//	goto done;
		//}

		// TODO: Handle disconnect properly, should halt sending more data until restored

		//BLADE_DISCONNECT_RQU_DESTROY(&rqu);
		goto done;
	} else if (!strcmp(method, BLADE_NETCAST_METHOD)) {
		blade_netcast_rqu_t *rqu;

		status = BLADE_NETCAST_RQU_PARSE(cmd_pool, request, &rqu);

		swclt_cmd_ctx_unlock(cmd_ctx);

		if (status) {
			ks_log(KS_LOG_ERROR, "Failed to parse netcast command: %s (%lu)", ks_handle_describe_ctx(cmd_ctx), status);
			goto done;
		}

		if (status = swclt_store_update(ctx->store, rqu)) {
			ks_log(KS_LOG_WARNING, "Failed to update nodestore from netcast command: %s (%lu)",
				ks_handle_describe_ctx(cmd_ctx), status);
			BLADE_NETCAST_RQU_DESTROY(&rqu);
			goto done;
		}

		ks_log(KS_LOG_INFO, "Updated nodestore with netcast command: %s", ks_handle_describe_ctx(cmd_ctx));
		BLADE_NETCAST_RQU_DESTROY(&rqu);
		goto done;
	} else if (!strcmp(method, BLADE_EXECUTE_METHOD)) {
		blade_execute_rqu_t *rqu;
		const char *key;

		status = BLADE_EXECUTE_RQU_PARSE(cmd_pool, request, &rqu);
		swclt_cmd_ctx_unlock(cmd_ctx);

		ks_log(KS_LOG_DEBUG, "Dispatching incoming blade execute request: %s to callback", ks_handle_describe(cmd));

		if (status) {
			ks_log(KS_LOG_WARNING, "Failed to parse execute payload: %s (%lu)", ks_handle_describe_ctx(cmd_ctx), status);
			goto done;
		}

		/* Look up the pmethod, and execute it */
		key = __make_pmethod_key(ctx, rqu->protocol, rqu->method);
		if (status = __execute_pmethod_cb(ctx, cmd_ctx, cmd_pool, ks_hash_search(ctx->methods, key, KS_UNLOCKED), rqu)) {
			ks_log(KS_LOG_ERROR, "Error executing pmethod: %lu", status);
		}
		ks_pool_free(&key);

		/* Done with the request */
		BLADE_EXECUTE_RQU_DESTROY(&rqu);

		if (!status) {
			ks_log(KS_LOG_INFO, "Sending reply back from execute request: %s", ks_handle_describe(cmd));

			/* Now the command is ready to be sent back, enqueue it */
			if (status = swclt_conn_submit_result(ctx->conn, cmd))
				ks_log(KS_LOG_ERROR, "Failed to submit reply from execute: %lu", status);
			else
				ks_log(KS_LOG_INFO, "Sent reply back from execute request: %s", ks_handle_describe(cmd));
		}
		goto done;
	} else {
		swclt_cmd_ctx_unlock(cmd_ctx);
		ks_log(KS_LOG_WARNING, "Not handling incoming command: %s", ks_handle_describe_ctx(cmd_ctx));
	}

done:
	/* Remove our reservation of the command */
	swclt_cmd_ctx_put(&cmd_ctx);

	/* Done with the command now (NOTE: Commands are children of the connections so
	 * if we return here without destroying it it will eventually get destroyed when
	 * the connection dies or the session reconnects) */
	ks_handle_destroy(&cmd);
	return status;
}

static ks_status_t __do_disconnect(swclt_sess_ctx_t *ctx)
{
	ks_handle_destroy(&ctx->conn);
	return KS_STATUS_SUCCESS;
}

static ks_status_t __on_connect_reply(swclt_conn_t conn, ks_json_t *error, const blade_connect_rpl_t *connect_rpl, swclt_sess_ctx_t *ctx)
{
	ks_status_t status = KS_STATUS_FAIL;

	if (error && ks_json_get_object_number_int_def(error, "code", 0) == -32002) {
		if (ctx->auth_failed_cb) ctx->auth_failed_cb(ctx->base.handle);
	}

    if (connect_rpl) {
		status = KS_STATUS_SUCCESS;
		if (!connect_rpl->session_restored)
		{
			/* Great we got the reply populate the node store */
			swclt_store_reset(ctx->store);
			if (status = swclt_store_populate(ctx->store, connect_rpl))
				ks_log(KS_LOG_WARNING, "Failed to populate node store from connect reply (%lu)", status);
		}
	}

	return status;
}

static void __on_conn_state_change(swclt_sess_ctx_t *ctx, swclt_hstate_change_t *state_change)
{
	/* Enqueue a state change on ourselves as well */
	char *reason = ks_psprintf(ctx->base.pool, "Connection failed: %s", swclt_hstate_describe_change(state_change));
	swclt_hstate_changed(&ctx->base, SWCLT_HSTATE_DEGRADED, KS_STATUS_FAIL, reason);
	ks_pool_free(&reason);

	/* Now we, as a session, will want to re-connect so, enqueue a request do to so */
	swclt_hstate_initiate_change_in(&ctx->base, SWCLT_HSTATE_ONLINE, __context_state_transition, 1000, 5000);
}

static ks_status_t __do_connect(swclt_sess_ctx_t *ctx)
{
	ks_status_t status;
	ks_json_t *authentication = NULL;

	/* Defer this check here, so it can be rescanned from ENV at runtime after session creation */
	if (!ctx->config->private_key_path || !ctx->config->client_cert_path) {
		if (!ctx->config->authentication) {
			ks_log(KS_LOG_ERROR, "Cannot connect without certificates or authentication");
			return KS_STATUS_FAIL;
		}
	}

	ks_log(KS_LOG_INFO, "Session is performing connect");

	/* Delete the previous connection if present */
	ks_handle_destroy(&ctx->conn);

	/* Re-allocate a new ssl context */
	if (status = __setup_ssl(ctx)) {
		ks_log(KS_LOG_CRIT, "SSL Setup failed: %lu", status);
		return status;
	}

	ks_log(KS_LOG_INFO, "Successfully setup ssl, initiating connection");

	if (ctx->config->authentication) {
		authentication = ks_json_parse(ctx->config->authentication);
	}

	/* Create a connection and have it call us back anytime a new read is detected */
	if (status = swclt_conn_connect_ex(
			&ctx->conn,
			(swclt_conn_incoming_cmd_cb_t)__on_incoming_cmd,
			ctx,
			(swclt_conn_connect_cb_t)__on_connect_reply,
			ctx,
			&ctx->ident,
			ctx->info.sessionid,			/* Pass in our session id, if it was previous valid we'll try to re-use it */
			&authentication,
			ctx->ssl)) {
		if (authentication) ks_json_delete(&authentication);
		return status;
	}

	if (status = swclt_conn_info(ctx->conn, &ctx->info.conn)) {
		ks_debug_break();	/* unexpected */
		ks_handle_destroy(&ctx->conn);
		return status;
	}

	/* If we got a new session id, stash it */
	if (!ks_uuid_is_null(&ctx->info.sessionid)) {
		if (ks_uuid_cmp(&ctx->info.sessionid, &ctx->info.conn.sessionid)) {
			ks_log(KS_LOG_WARNING, "New session id created (old: %s, new: %s), all state invalidated",
				ks_uuid_thr_str(&ctx->info.sessionid), ks_uuid_thr_str(&ctx->info.conn.sessionid));
			ctx->base.state = SWCLT_HSTATE_NORMAL;
		}
	}

	/* @@ TODO invalidate all state **/
	ctx->info.sessionid = ctx->info.conn.sessionid;
	ctx->info.nodeid = ks_pstrdup(ctx->base.pool, ctx->info.conn.nodeid);
	ctx->info.master_nodeid = ks_pstrdup(ctx->base.pool, ctx->info.conn.master_nodeid);

	/* Monitor for state changed on the connection */
	swclt_hstate_register_listener(&ctx->base, __on_conn_state_change, ctx->conn);

	ks_log(KS_LOG_INFO, "Successfully established sessionid: %s", ks_uuid_thr_str(&ctx->info.sessionid));
	ks_log(KS_LOG_INFO, "   nodeid: %s", ctx->info.nodeid);
	ks_log(KS_LOG_INFO, "   master_nodeid: %s", ctx->info.master_nodeid);

	return status;
}

static void __context_describe(swclt_sess_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	const char *desc = NULL;
	if (ctx->conn && (desc = ks_handle_describe(ctx->conn))) {
		/* We have to do all this garbage because of the poor decision to nest ks_handle_describe() calls that return a common thread local buffer */
		ks_size_t desc_len = strlen(desc);
		const char *preamble = "SWCLT Session - ";
		ks_size_t preamble_len = strlen(preamble);
		if (desc_len + preamble_len + 1 > buffer_len) {
			desc_len = buffer_len - preamble_len - 1;
		}
		memmove(buffer + preamble_len, desc, desc_len + 1);
		memcpy(buffer, preamble, preamble_len);
		buffer[buffer_len - 1] = '\0';
	} else {
		snprintf(buffer, buffer_len, "SWCLT Session (not connected)");
	}
}

static ks_status_t __context_state_transition(swclt_sess_ctx_t *ctx, SWCLT_HSTATE new_state)
{
	switch (new_state) {
		case SWCLT_HSTATE_ONLINE:
			return __do_connect(ctx);

		case SWCLT_HSTATE_OFFLINE:
			return __do_disconnect(ctx);

		default:
			ks_debug_break();
			return KS_STATUS_SUCCESS;
	}
}

static ks_status_t __disconnect(swclt_sess_ctx_t *ctx)
{
	/* If we're already degraded null op */
	if (swclt_hstate_get_ctx(&ctx->base) == SWCLT_HSTATE_OFFLINE)
		return KS_STATUS_SUCCESS;

	/* Request a state transition to offline */
	swclt_hstate_initiate_change_now(&ctx->base, SWCLT_HSTATE_OFFLINE, __context_state_transition, 1000);
	return KS_STATUS_SUCCESS;
}

static ks_status_t __connect(swclt_sess_ctx_t *ctx)
{
	/* If we're already ready null op */
	if (swclt_hstate_get_ctx(&ctx->base) == SWCLT_HSTATE_ONLINE)
		return KS_STATUS_SUCCESS;

	/* Right so our state is still degraded, time to connect */
	swclt_hstate_initiate_change_now(&ctx->base, SWCLT_HSTATE_ONLINE, __context_state_transition, 5000);
	return KS_STATUS_SUCCESS;
}

static ks_status_t __swclt_sess_metric_register(swclt_sess_ctx_t *ctx, const char *protocol, int interval, int rank)
{
	swclt_metric_reg_t *reg = NULL;

	ks_hash_write_lock(ctx->metrics);

	reg = (swclt_metric_reg_t *)ks_hash_search(ctx->metrics, (const void *)protocol, KS_UNLOCKED);
	if (reg) {
		ks_log(KS_LOG_INFO, "Metric update for '%s'\n", protocol);
		reg->interval = interval;
		reg->rank = rank;
	} else {
		ks_log(KS_LOG_INFO, "Metric added for '%s'\n", protocol);

		reg = (swclt_metric_reg_t *)ks_pool_alloc(ks_pool_get(ctx->metrics), sizeof(swclt_metric_reg_t));
		reg->timeout = ks_time_now();
		reg->interval = interval;
		reg->rank = rank;
		reg->dirty = KS_TRUE;

		ks_hash_insert(ctx->metrics, (const void *)ks_pstrdup(ks_pool_get(ctx->metrics), protocol), (void *)reg);
	}

	ks_hash_write_unlock(ctx->metrics);


	return KS_STATUS_SUCCESS;
}

static ks_status_t __swclt_sess_metric_update(swclt_sess_ctx_t *ctx, const char *protocol, int rank)
{
	swclt_metric_reg_t *reg = NULL;

	ks_hash_write_lock(ctx->metrics);

	reg = (swclt_metric_reg_t *)ks_hash_search(ctx->metrics, (const void *)protocol, KS_UNLOCKED);
	if (reg && reg->rank != rank) {
		reg->rank = rank;
		reg->dirty = KS_TRUE;
	}

	ks_hash_write_unlock(ctx->metrics);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __swclt_sess_metric_current(swclt_sess_ctx_t *ctx, const char *protocol, int *rank)
{
	ks_status_t ret = KS_STATUS_NOT_FOUND;
	swclt_metric_reg_t *reg = NULL;

	ks_hash_read_lock(ctx->metrics);

	reg = (swclt_metric_reg_t *)ks_hash_search(ctx->metrics, (const void *)protocol, KS_UNLOCKED);
	if (reg) {
		*rank = reg->rank;
		ret = KS_STATUS_SUCCESS;
	}

	ks_hash_read_unlock(ctx->metrics);

	return ret;
}

static void __context_service(swclt_sess_ctx_t *ctx)
{
	ks_hash_read_lock(ctx->metrics);
	for (ks_hash_iterator_t *itt = ks_hash_first(ctx->metrics, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
		const char *key = NULL;
		swclt_metric_reg_t *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		if (ks_time_now() >= value->timeout && value->dirty) {
			value->timeout = ks_time_now() + (value->interval * KS_USEC_PER_SEC);
			value->dirty = KS_FALSE;

			swclt_sess_protocol_provider_rank_update_async(ctx->base.handle, key, value->rank, NULL, NULL, NULL);
		}
	}
	ks_hash_read_unlock(ctx->metrics);

	/* Now ask to be serviced again in 1 second */
	swclt_hmgr_request_service_in(&ctx->base, 1000);
}

static ks_status_t __context_init(
	swclt_sess_ctx_t *ctx,
	const char *identity_uri,
	swclt_config_t *config)
{
	ks_status_t status;

	ks_assert(ctx->base.state == SWCLT_HSTATE_NORMAL);

	ks_log(KS_LOG_INFO, "Session created with identity uri: %s", identity_uri);

	/* Allow the config to be shared, no ownership change */
	ctx->config = config;

	/* Parse the identity, it will contain the connection target address etc. */
	if (status = swclt_ident_from_str(&ctx->ident, ctx->base.pool, identity_uri)) {
		ks_log(KS_LOG_ERROR, "Invalid identity uri: %s", identity_uri);
		goto done;
	}

	/* Allocate the subscriptions hash */
	if (status = ks_hash_create(
			&ctx->subscriptions,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_MUTEX,
			ctx->base.pool))
		goto done;

	/* Allocate the methods hash */
	if (status = ks_hash_create(
			&ctx->methods,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_RWLOCK,
			ctx->base.pool))
		goto done;

	if (status = ks_hash_create(
			&ctx->setups,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_RWLOCK,
			ctx->base.pool))
		goto done;

	if (status = ks_hash_create(
			&ctx->metrics,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_RWLOCK,
			ctx->base.pool))
		goto done;

	/* Verify the config has what we need */
	if (!ctx->config->private_key_path || !ctx->config->client_cert_path) {
		if (!ctx->config->authentication) {
			ks_log(KS_LOG_WARNING, "No authentication configured");
		}
	}

	if (status = swclt_store_create(&ctx->store)) {
		ks_log(KS_LOG_ERROR, "Failed to initialize node store (%lu)", status);
		goto done;
	}

	ks_handle_set_parent(ctx->store, ctx->base.handle);

	/* One second pulsing service */
	swclt_hstate_register_service(&ctx->base, __context_service);

done:
	if (status)
		__context_deinit(ctx);

	return status;
}

static ks_status_t __nodeid_local(swclt_sess_t sess, const char *nodeid)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = !strcmp(ctx->info.nodeid, nodeid) ? KS_STATUS_SUCCESS : KS_STATUS_FAIL;
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

static ks_handle_t * __dupe_handle(swclt_sess_ctx_t *ctx, ks_handle_t handle)
{
	ks_handle_t *dup = ks_pool_alloc(ctx->base.pool, sizeof(handle));
	ks_assertd(dup);
	memcpy(dup, &handle, sizeof(handle));
	return dup;
}

static ks_status_t __unregister_subscription(
	swclt_sess_ctx_t *ctx,
	const char *protocol,
	const char *channel)
{
	const char *key = __make_subscription_key(ctx, protocol, channel);
	ks_handle_t *sub = (ks_handle_t *)ks_hash_remove(ctx->subscriptions, key);
	ks_pool_free(&key);
	if (!sub)
		return KS_STATUS_NOT_FOUND;

	ks_handle_destroy(sub);
	ks_pool_free(&sub);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __register_subscription(
	swclt_sess_ctx_t *ctx,
	const char * protocol,
	const char * channel,
	swclt_sub_t sub)
{
	/* Mark this subscription as a dependent */
	ks_handle_set_parent(sub, ctx->base.handle);

	/* unregister if already registered so it does not leak anything, even the handle for the sub
	 * should be cleaned up to avoid leaking for the duration of the session */
	__unregister_subscription(ctx, protocol, channel);

	/* @todo why duplicate the handle on the heap? can assign value of handle to the void* and not use FREE_VALUE hash flag
	 * answer: check out the sub, then put it in the hash */

	/* And add it to the hash */
	return ks_hash_insert(ctx->subscriptions, __make_subscription_key(ctx, protocol, channel), __dupe_handle(ctx, sub));
}

static ks_status_t __register_pmethod(
	swclt_sess_ctx_t *ctx,
	const char *protocol,
	const char *method,
	swclt_pmethod_cb_t pmethod,
	void *cb_data)
{
	if (!pmethod) {
		const char *key = __make_pmethod_key(ctx, protocol, method);
		ks_hash_remove(ctx->methods, key);
		ks_pool_free(&key);
		return KS_STATUS_SUCCESS;
	}
	return ks_hash_insert(ctx->methods, __make_pmethod_key(ctx, protocol, method), __make_pmethod_value(ctx, pmethod, cb_data));
}

static ks_status_t __lookup_setup(swclt_sess_ctx_t *ctx, const char *service, ks_pool_t *pool, char **protocol)
{
	ks_status_t status = KS_STATUS_NOT_FOUND;
	ks_bool_t exists = KS_FALSE;
	const char *proto = NULL;

	ks_hash_read_lock(ctx->setups);
	proto = ks_hash_search(ctx->setups, service, KS_UNLOCKED);
	if (proto) {
		*protocol = ks_pstrdup(pool, proto);
		status = KS_STATUS_SUCCESS;
	}
	ks_hash_read_unlock(ctx->setups);

	return status;
}

static ks_status_t __register_setup(swclt_sess_ctx_t *ctx, const char *service, const char *protocol)
{
	ks_status_t status;

	ks_hash_write_lock(ctx->setups);
	status = ks_hash_insert(ctx->setups, ks_pstrdup(ks_pool_get(ctx->setups), service), ks_pstrdup(ks_pool_get(ctx->setups), protocol));
	ks_hash_write_unlock(ctx->setups);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_set_auth_failed_cb(swclt_sess_t sess, swclt_sess_auth_failed_cb_t cb)
{
	swclt_sess_ctx_t *ctx;
	ks_status_t result = KS_STATUS_SUCCESS;

	if(ks_handle_get(SWCLT_HTYPE_SESS, sess, &ctx))
		return result;

	ctx->auth_failed_cb = cb;

done:
	ks_handle_put(SWCLT_HTYPE_SESS, &ctx);

	return result;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_target_set(swclt_sess_t sess, const char *target)
{
	swclt_sess_ctx_t *ctx;
	ks_status_t result = KS_STATUS_SUCCESS;

	if(ks_handle_get(SWCLT_HTYPE_SESS, sess, &ctx))
		return result;

	/* Parse the identity, it will contain the connection target address etc. */
	if (result = swclt_ident_from_str(&ctx->ident, ctx->base.pool, target)) {
		ks_log(KS_LOG_ERROR, "Invalid identity uri: %s", target);
		goto done;
	}
	ks_log(KS_LOG_ERROR, "Updated session target to %s", target);

done:
	ks_handle_put(SWCLT_HTYPE_SESS, &ctx);

	return result;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_register(swclt_sess_t sess, const char *protocol, int interval, int rank)
{
	ks_assert(protocol);
	ks_assert(interval >= 1);
	ks_assert(rank >= 0);

	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = __swclt_sess_metric_register(ctx, protocol, interval, rank);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_update(swclt_sess_t sess, const char *protocol, int rank)
{
	ks_assert(protocol);
	ks_assert(rank >= 0);

	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = __swclt_sess_metric_update(ctx, protocol, rank);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_current(swclt_sess_t sess, const char *protocol, int *rank)
{
	ks_assert(protocol);

	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = __swclt_sess_metric_current(ctx, protocol, rank);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) __swclt_sess_create(
	swclt_sess_t *sess,
	const char *identity_uri,
	swclt_config_t *config,
	const char *file,
	int line,
	const char *tag)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M_TAG(
		NULL,
		file,
		line,
		tag,
		SWCLT_HTYPE_SESS,
		sess,
		swclt_sess_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		identity_uri,
		config);
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_has_authentication(swclt_sess_t sess)
{
	swclt_sess_ctx_t *ctx;
	ks_bool_t result = KS_FALSE;

	if(ks_handle_get(SWCLT_HTYPE_SESS, sess, &ctx))
		return result;

	result = (ctx->config->private_key_path && ctx->config->client_cert_path) || ctx->config->authentication;

	ks_handle_put(SWCLT_HTYPE_SESS, &ctx);

	return result;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_connect(swclt_sess_t sess)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = __connect(ctx);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_disconnect(swclt_sess_t sess)
{
	ks_log(KS_LOG_DEBUG, "Disconnecting");
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = __disconnect(ctx);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
	ks_log(KS_LOG_DEBUG, "Disconnected");
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_connected(swclt_sess_t sess)
{
	swclt_sess_ctx_t *ctx;
	ks_bool_t result = KS_FALSE;

	if(ks_handle_get(SWCLT_HTYPE_SESS, sess, &ctx))
		return result;

	result = __session_check_connected(ctx, KS_FALSE);

	ks_handle_put(SWCLT_HTYPE_SESS, &ctx);

	return result;
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_restored(swclt_sess_t sess)
{
	swclt_handle_base_t *base;
	SWCLT_HSTATE state;
	SWCLT_HSTATE last_state;

	if (!swclt_htype_valid(KS_HANDLE_TYPE_FROM_HANDLE(sess)))
		return KS_FALSE;

	if(ks_handle_get(0, sess, &base))
		return KS_FALSE;

	ks_spinlock_acquire(&base->lock);
	state = base->state;
	last_state = base->last_state;
	if (base->pending_state_change_service != SWCLT_HSTATE_INVALID)
		state = SWCLT_HSTATE_INVALID;
	ks_spinlock_release(&base->lock);

	ks_handle_put(0, &base);

	return state == SWCLT_HSTATE_ONLINE && last_state == SWCLT_HSTATE_DEGRADED;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_info(
	swclt_sess_t sess,
   	ks_pool_t *pool,
	ks_uuid_t *sessionid,
	char **nodeid,
	char **master_nodeid)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);

	if (__session_check_connected(ctx, KS_TRUE)) {

		/* Default to our pool if no pool specified */
		if (!pool)
			pool = ctx->base.pool;

		/* Context will be locked now, safe the access info */
		if (sessionid)
			*sessionid = ctx->info.sessionid;
		if (nodeid)
			*nodeid = ks_pstrdup(pool, ctx->info.nodeid);
		if (master_nodeid)
			*master_nodeid = ks_pstrdup(pool, ctx->info.master_nodeid);

		/* Now unlock the context */
		ks_spinlock_release(&ctx->base.lock);
	}

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_nodeid(swclt_sess_t sess, ks_pool_t *pool, char **nodeid)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	*nodeid = ks_pstrdup(pool, ctx->info.nodeid);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_nodeid_local(swclt_sess_t sess, const char *nodeid)
{
	return swclt_sess_connected(sess) && !__nodeid_local(sess, nodeid);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_nodestore(
	swclt_sess_t sess,
	swclt_store_t *store)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	*store = ctx->store;
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) __swclt_sess_register_protocol_method(
	swclt_sess_t sess,
	const char *protocol,
	const char *method,
	swclt_pmethod_cb_t pmethod_cb,
	void *cb_data)
{
	/* @todo consider changing pmethod to a handle, and internally store both the callback and a user data pointer
	 * however, userdata associated at the time of pmethod registration does not make sense for conceivable use cases */
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = __register_pmethod(ctx, protocol, method, pmethod_cb, cb_data);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_register_subscription_method(
	swclt_sess_t sess,
	swclt_sub_t *sub,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);

	if (status = swclt_sub_create(sub, protocol, channel, cb, cb_data))
		goto ks_handle_scope_end;

	/* Register this subscription, if request fails we just won't receive the events,
	 * but registering the callback is fine and can be replaced if client tries again */
	if (status = __register_subscription(ctx, protocol, channel, *sub)) {
		ks_handle_destroy(sub);
		goto ks_handle_scope_end;
	}

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_broadcast(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	const char *event,
	ks_json_t **params)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_BROADCAST_CMD(
			protocol,
			channel,
			event,
			ctx->info.nodeid,
			params))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* Now submit it */
	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

done:
	ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_get_rates(swclt_sess_t sess, ks_throughput_t *recv, ks_throughput_t *send)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	status = swclt_conn_get_rates(ctx->conn, recv, send);
	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_sub_t *sub,
	swclt_cmd_t *cmdP)
{
	return swclt_sess_subscription_add_async(
		sess,
		protocol,
		channel,
		cb,
		cb_data,
		sub,
		NULL,		/* By passing null here we make this synchronous by implication */
		NULL,
		cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add_async(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_sub_t *sub,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;
	blade_subscription_rpl_t *subscription_rpl = NULL;

	/* @todo remove the next 2 calls, and call swclt_sess_register_subscription_method externally, and
	 * update to allow multiple channels to be subscribed via ks_json_t array of channel name strings
	 * can also verify that the callbacks are already registered here as a sanity check */

	/* We also will track this subscription with a handle */
	if (status = swclt_sub_create(sub, protocol, channel, cb, cb_data))
		goto done;

	/* Register this subscription, if request fails we just won't receive the events,
	 * but registering the callback is fine and can be replaced if client tries again */
	if (status = __register_subscription(ctx, protocol, channel, *sub))
		goto done;

	/* Allocate the request */
	if (!(cmd = CREATE_BLADE_SUBSCRIPTION_CMD(
			BLADE_SUBSCRIPTION_CMD_ADD,
			protocol,
			channel))) {
		goto done;
	}

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	/* Now submit the command */
	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

	/* If not async, and caller didnt pass in a spot for the command, destroy it */
	if (!cmdP && !response_callback)
		ks_handle_destroy(&cmd);
	else {
		if (cmdP) *cmdP = cmd;

		/* Parent it to connection for safe guard */
		ks_handle_set_parent(cmd, ctx->conn);
	}

done:
	/* Only destroy the subscription if we failed */
	if (status) {
		ks_handle_destroy(&cmd);
		ks_handle_destroy(sub);
	}

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_t *cmdP)
{
	return swclt_sess_subscription_remove_async(
		sess,
		protocol,
		channel,
		NULL,		/* By passing null here we make this synchronous by implication */
		NULL,
		cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove_async(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	/* Unregister this subscription if it exists, if not then just continue with subscription removal
	 * because the callback may have been removed manually */
	__unregister_subscription(ctx, protocol, channel);

	/* Allocate the request */
	if (!(cmd = CREATE_BLADE_SUBSCRIPTION_CMD(
			BLADE_SUBSCRIPTION_CMD_REMOVE,
			protocol,
			channel))) {
		goto done;
	}

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	/* Now submit the command */
	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

	/* If not async, and caller didnt pass in a spot for the command, destroy it */
	if (!cmdP && !response_callback)
		ks_handle_destroy(&cmd);
	else {
		if (cmdP) *cmdP = cmd;

		/* Parent it to connection for safe guard */
		ks_handle_set_parent(cmd, ctx->conn);
	}

done:
	if (status)
		ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add(
	swclt_sess_t sess,
	const char * protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
	ks_json_t **channels,
	int rank,
	ks_json_t **data,
	swclt_cmd_t *cmdP)
{
	return swclt_sess_protocol_provider_add_async(
		sess,
		protocol,
		default_method_execute_access,
		default_channel_subscribe_access,
		default_channel_broadcast_access,
		methods,
		channels,
		rank,
		data,
		NULL,		/* By passing null here we make this synchronous by implication */
		NULL,
		cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add_async(
	swclt_sess_t sess,
	const char * protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
	ks_json_t **channels,
	int rank,
	ks_json_t **data,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD(
			protocol,
			default_method_execute_access,
			default_channel_subscribe_access,
			default_channel_broadcast_access,
			methods,
			channels,
			rank,
			data))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* @todo verify that the subscription callbacks are already registered here as a sanity check
	 * for any channels that are auto_subscribed */

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	/* Now hand it to the connection to submit it, it will block until the
	 * reply is received */
	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

	/* If not async, and caller didnt pass in a spot for the command, destroy it */
	if (!cmdP && !response_callback)
		ks_handle_destroy(&cmd);
	else {
		if (cmdP) *cmdP = cmd;

		/* Parent it to connection for safe guard */
		ks_handle_set_parent(cmd, ctx->conn);
	}

done:
	if (status)
		ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove(
	swclt_sess_t sess,
   	const char * protocol,
	swclt_cmd_t *cmdP)
{
	return swclt_sess_protocol_provider_remove_async(
		sess,
		protocol,
		NULL,		/* By passing null here we make this synchronous by implication */
		NULL,
		cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove_async(
	swclt_sess_t sess,
	const char * protocol,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	/* @todo separate protocol command field parsing into different types based on the command? */

	/* Create the command */
	if (!(cmd = CREATE_BLADE_PROTOCOL_PROVIDER_REMOVE_CMD(protocol))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	/* Now hand it to the connection to submit it, it will block until the
	 * reply is received */
	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

	/* If not async, and caller didnt pass in a spot for the command, destroy it */
	if (!cmdP && !response_callback)
		ks_handle_destroy(&cmd);
	else {
		if (cmdP) *cmdP = cmd;

		/* Parent it to connection for safe guard */
		ks_handle_set_parent(cmd, ctx->conn);
	}

done:
	if (status)
		ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update(
	swclt_sess_t sess,
	const char * protocol,
	int rank,
	swclt_cmd_t *cmdP)
{
	return swclt_sess_protocol_provider_rank_update_async(
		sess,
		protocol,
		rank,
		NULL,		/* By passing null here we make this synchronous by implication */
		NULL,
		cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update_async(
	swclt_sess_t sess,
	const char * protocol,
	int rank,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_CMD(protocol, rank))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	/* Now hand it to the connection to submit it, it will block until the
	 * reply is received */
	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

	/* If not async, and caller didnt pass in a spot for the command, destroy it */
	if (!cmdP && !response_callback)
		ks_handle_destroy(&cmd);
	else {
		if (cmdP) *cmdP = cmd;

		/* Parent it to connection for safe guard */
		ks_handle_set_parent(cmd, ctx->conn);
	}

done:
	if (status)
		ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}


SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add(
	swclt_sess_t sess,
	const char *identity,
	swclt_cmd_t *cmdP)
{
	return swclt_sess_identity_add_async(
		sess,
		identity,
		NULL,		/* By passing null here we make this synchronous by implication */
		NULL,
		cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add_async(
	swclt_sess_t sess,
	const char *identity,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_IDENTITY_CMD(
			BLADE_IDENTITY_CMD_ADD,
			identity))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	/* Now hand it to the connection to submit it, it will block until the
	 * reply is received */
	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

	/* If not async, and caller didnt pass in a spot for the command, destroy it */
	if (!cmdP && !response_callback)
		ks_handle_destroy(&cmd);
	else {
		if (cmdP) *cmdP = cmd;
		ks_handle_set_parent(cmd, ctx->conn);
	}

done:
	if (status)
	   	ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_execute(
	swclt_sess_t sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_t *cmdP)
{
	return swclt_sess_execute_async(
		sess,
		responder,
		protocol,
		method,
		params,
		NULL,		/* By passing null here we make this synchronous by implication */
		NULL,
		cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_async(
	swclt_sess_t sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_EXECUTE_CMD(
			responder,
			protocol,
			method,
			params))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	if (status = swclt_conn_submit_request(ctx->conn, cmd))
		goto done;

	/* If not async, and caller didnt pass in a spot for the command, destroy it */
	if (!cmdP && !response_callback)
		ks_handle_destroy(&cmd);
	else {
		if (cmdP) *cmdP = cmd;
		ks_handle_set_parent(cmd, ctx->conn);
	}

done:
	if (status)
		ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

// @todo everything below here is high level supporting stuff related to future IDL templates
// and should probably be moved to it's own separate file for each protocol supported

// signalwire consumer

SWCLT_DECLARE(ks_status_t) swclt_sess_signalwire_setup(swclt_sess_t sess, const char *service, swclt_sub_cb_t cb, void *cb_data)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);

	swclt_store_t store;
	ks_pool_t *pool = NULL;
	ks_json_t *params = NULL;
	swclt_cmd_t cmd = KS_NULL_HANDLE;
	SWCLT_CMD_TYPE cmd_type;
	ks_json_t *result = NULL;
	const char *protocol = NULL;
	ks_bool_t instance_found = KS_FALSE;
	swclt_sub_t sub;

	ks_assert(service);
	ks_assert(cb);

	// Make sure we are at least connected
	if (!swclt_sess_connected(sess)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' failed because session is not connected", service);
		status = KS_STATUS_INACTIVE;
		goto done;
	}

	// And make sure the nodestore is available
	if (status = swclt_sess_nodestore(sess, &store)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' failed because session nodestore is unavailable: %d", service, status);
		goto done;
	}

	pool = ks_handle_pool(sess);

	params = ks_json_pcreate_object(pool);
	ks_json_padd_string_to_object(pool, params, "service", service);

	// Send the setup request syncronously, if it fails bail out
	if (status = swclt_sess_execute(sess,
									NULL,
									"signalwire",
									"setup",
									&params,
									&cmd)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' execute failed: %d", service, status);
		goto done;
	}

	// Make sure the command is valid...
	if (status = swclt_cmd_type(cmd, &cmd_type)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' command invalid: %d", service, status);
		goto done;
	}

	// If we get an error back, bail out
	if (cmd_type == SWCLT_CMD_TYPE_ERROR) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' response type error", service);
		// @todo log more details from the error response
		status = KS_STATUS_FAIL;
		goto done;
	}

	// If it's not an error or a result, what is it, did someone forget to set it from the request?
	if (cmd_type != SWCLT_CMD_TYPE_RESULT) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' response type invalid", service);
		status = KS_STATUS_FAIL;
		goto done;
	}

	// Make sure we actually get a result to look at
	if (status = swclt_cmd_result(cmd, (const ks_json_t **)&result)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' response has no result: %d", service, status);
		goto done;
	}

	// Get the inner result of the execute
	result = ks_json_get_object_item(result, "result");
	if (!result) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' response has no result.result", service);
		status = KS_STATUS_ARG_NULL;
		goto done;
	}

	// Get protocol from result, duplicate it so we can destroy the command
	protocol = ks_json_get_object_cstr_def(result, "protocol", NULL);
	if (protocol) protocol = ks_pstrdup(ks_handle_pool(sess), protocol);

	if (!protocol) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' response has no result.result.protocol", service);
		status = KS_STATUS_ARG_NULL;
		goto done;
	}

	// Destroy the execute command, taking the result data with it, protocol is duplicated in session pool
	ks_handle_destroy(&cmd);

	ks_log(KS_LOG_DEBUG, "Setup for '%s' waiting for provider of protocol instance: %s", service, protocol);

	// poll nodestore until protocol is seen locally (or timeout after 2 seconds of waiting), which
	// ensures our upstream also sees it which means we can subscribe to the channel without failing
	// if it's not known by upstream yet
	{
		int nodestore_attempts = 20;
		while (!instance_found && nodestore_attempts) {
			if (!(instance_found = !swclt_store_check_protocol(store, protocol))) {
				ks_sleep_ms(100);
				--nodestore_attempts;
			}
		}
	}

	// If we didn't see the protocol, we timed out waiting, gandalf issue?
	if (!instance_found) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' protocol instance timeout", service);
		status = KS_STATUS_TIMEOUT;
		goto done;
	}

	// Now that protocol is available, sync subscribe to the notifications channel
	if (status = swclt_sess_subscription_add(sess, protocol, "notifications", cb, cb_data, &sub, &cmd)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' subscription add failed: %d", service, status);
		goto done;
	}

	// Make sure the command is valid...
	if (status = swclt_cmd_type(cmd, &cmd_type)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' subscription add command invalid: %d", service, status);
		goto done;
	}

	// If we get an error back, bail out
	if (cmd_type == SWCLT_CMD_TYPE_ERROR) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' subscription add response type error", service);
		// @todo log more details from the error response
		status = KS_STATUS_FAIL;
		goto done;
	}

	// If it's not an error or a result, what is it, did someone forget to set it from the request?
	if (cmd_type != SWCLT_CMD_TYPE_RESULT) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' subscription add response type invalid", service);
		status = KS_STATUS_FAIL;
		goto done;
	}

	// Make sure we actually get a result to look at
	if (status = swclt_cmd_result(cmd, (const ks_json_t **)&result)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s', subscription add response has no result: %d", service, status);
		goto done;
	}

	// Destroy the subscription add command
	ks_handle_destroy(&cmd);

	__register_setup(ctx, service, protocol);

done:
	if (protocol) ks_pool_free(&protocol);
	if (ks_handle_valid(cmd)) ks_handle_destroy(&cmd);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
}

// signalwire provisioning consumer

SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_setup(swclt_sess_t sess, swclt_sub_cb_t cb, void *cb_data)
{
	return swclt_sess_signalwire_setup(sess, "provisioning", cb, cb_data);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure(swclt_sess_t sess,
															 const char *target,
															 const char *local_endpoint,
															 const char *external_endpoint,
															 const char *relay_connector_id,
															 swclt_cmd_t *cmdP)
{
	return swclt_sess_provisioning_configure_async(sess,
												   target,
												   local_endpoint,
												   external_endpoint,
												   relay_connector_id,
												   NULL,		/* By passing null here we make this synchronous by implication */
												   NULL,
												   cmdP);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure_async(swclt_sess_t sess,
																   const char *target,
																   const char *local_endpoint,
																   const char *external_endpoint,
																   const char *relay_connector_id,
																   swclt_cmd_cb_t response_callback,
																   void *response_callback_data,
																   swclt_cmd_t *cmdP)
{
	SWCLT_SESS_SCOPE_BEG(sess, ctx, status);

	swclt_store_t store;
	ks_pool_t *pool = NULL;
	char *protocol = NULL;
	ks_json_t *params = NULL;

	ks_assert(target);
	ks_assert(local_endpoint);
	ks_assert(external_endpoint);
	ks_assert(relay_connector_id);

	if (!swclt_sess_connected(sess)) {
		goto done;
	}

	if (status = swclt_sess_nodestore(sess, &store))
		goto done;

	pool = ks_handle_pool(sess);

	if (__lookup_setup(ctx, "provisioning", pool, &protocol)) {
		ks_log(KS_LOG_ERROR, "Provisioning setup has not been performed");
		status = KS_STATUS_FAIL;
		goto done;
	}

	params = ks_json_pcreate_object(pool);

	ks_json_padd_string_to_object(pool, params, "target", target);
	ks_json_padd_string_to_object(pool, params, "local_endpoint", local_endpoint);
	ks_json_padd_string_to_object(pool, params, "external_endpoint", external_endpoint);
	ks_json_padd_string_to_object(pool, params, "relay_connector_id", relay_connector_id);

	status = swclt_sess_execute_async(sess,
									  NULL,
									  protocol,
									  "configure",
									  &params,
									  response_callback,
									  response_callback_data,
									  cmdP);
done:
	if (protocol) ks_pool_free(&protocol);

	SWCLT_SESS_SCOPE_END(sess, ctx, status);
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
