/*
 * Copyright (c) 2018-2022 SignalWire, Inc
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

typedef struct swclt_metric_reg_s {
	int interval;
	ks_time_t timeout;
	int rank;
	ks_bool_t dirty;
} swclt_metric_reg_t;

typedef struct swclt_result_queue {
	swclt_cmd_t *result;
	struct swclt_result_queue *next;
	ks_time_t expire_time;
} swclt_result_queue_t;

static ks_status_t __do_connect(swclt_sess_t *sess);
static ks_status_t __do_disconnect(swclt_sess_t *sess);

static void enqueue_result(swclt_sess_t *sess, swclt_cmd_t *cmd)
{
	ks_mutex_lock(sess->result_mutex);
	swclt_result_queue_t *node = ks_pool_alloc(sess->pool, sizeof(*node));
	node->result = swclt_cmd_duplicate(cmd);
	node->expire_time = ks_time_now_sec() + 5;
	if (sess->result_last) {
		sess->result_last->next = node;
	}
	sess->result_last = node;
	if (!sess->result_first) {
		sess->result_first = node;
	}
	ks_mutex_unlock(sess->result_mutex);
}

static void dequeue_result(swclt_sess_t *sess, swclt_cmd_t **cmd)
{
	int retry;
	do {
		retry = 0;
		ks_mutex_lock(sess->result_mutex);
		swclt_result_queue_t *first = sess->result_first;
		*cmd = NULL;
		if (first) {
			if (first->expire_time >= ks_time_now_sec()) {
				*cmd = first->result;
			} else {
				// too old... discard it and retry with next result
				swclt_cmd_destroy(&first->result);
				retry = 1;
			}
			sess->result_first = first->next;
			ks_pool_free(&first);
		}
		if (!sess->result_first) {
			sess->result_last = NULL;
		}
		ks_mutex_unlock(sess->result_mutex);
	} while (retry);
}

static void submit_results(swclt_sess_t *sess)
{
	swclt_cmd_t *result = NULL;
	dequeue_result(sess, &result);
	while (result) {
		ks_rwl_read_lock(sess->rwlock);
		if (swclt_conn_submit_result(sess->conn, result) == KS_STATUS_DISCONNECTED) {
			ks_rwl_read_unlock(sess->rwlock);
			enqueue_result(sess, result);
			break;
		}
		ks_rwl_read_unlock(sess->rwlock);
		swclt_cmd_destroy(&result);
	}
}

static void flush_results(swclt_sess_t *sess)
{
	swclt_cmd_t *result = NULL;
	dequeue_result(sess, &result);
	while (result) {
		swclt_cmd_destroy(&result);
		dequeue_result(sess, &result);
	}
}

static void check_session_state(swclt_sess_t *sess)
{
	ks_hash_read_lock(sess->metrics);
	for (ks_hash_iterator_t *itt = ks_hash_first(sess->metrics, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
		const char *key = NULL;
		swclt_metric_reg_t *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		if (ks_time_now() >= value->timeout && value->dirty) {
			value->timeout = ks_time_now() + (value->interval * KS_USEC_PER_SEC);
			value->dirty = KS_FALSE;

			swclt_sess_protocol_provider_rank_update_async(sess, key, value->rank, NULL, NULL, NULL);
		}
	}
	ks_hash_read_unlock(sess->metrics);

	if (sess->disconnect_time > 0 && ks_time_now_sec() >= sess->disconnect_time) {
		sess->disconnect_time = 0;
		if (sess->state == SWCLT_STATE_ONLINE) {
			// Disconnect now and report state change
			__do_disconnect(sess);
			sess->state = SWCLT_STATE_OFFLINE;
			if (sess->state_change_cb) {
				sess->state_change_cb(sess, sess->state_change_cb_data);
			}
		}
	}
	if (sess->connect_time > 0 && ks_time_now_sec() >= sess->connect_time) {
		// Connect and report state change on success
		if (__do_connect(sess) == KS_STATUS_SUCCESS) {
			sess->connect_time = 0;
			if (sess->state_change_cb) {
				sess->state_change_cb(sess, sess->state_change_cb_data);
			}
		} else {
			// try again in 2 seconds
			sess->connect_time = ks_time_now_sec() + 2;
		}
	}
}

static void *session_monitor_thread(ks_thread_t *thread, void *data)
{
	swclt_sess_t *sess = (swclt_sess_t *)data;

	ks_log(KS_LOG_DEBUG, "Session monitor starting");
	while (ks_thread_stop_requested(thread) == KS_FALSE) {
		ks_cond_lock(sess->monitor_cond);
		ks_cond_timedwait(sess->monitor_cond, 1000);
		ks_cond_unlock(sess->monitor_cond);
		if (ks_thread_stop_requested(thread) == KS_FALSE) {
			check_session_state(sess);
		}
	}

	ks_log(KS_LOG_DEBUG, "Session monitor thread stopping");
	return NULL;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_destroy(swclt_sess_t **sessP)
{
	if (sessP && *sessP) {
		swclt_sess_t *sess = *sessP;
		ks_pool_t *pool = sess->pool;
		*sessP = NULL;
		if (sess->monitor_thread) {
			if (ks_thread_request_stop(sess->monitor_thread) != KS_STATUS_SUCCESS) {
				ks_log(KS_LOG_ERROR, "Failed to stop session monitor thread.  Leaking data and moving on.");
				return KS_STATUS_FAIL;
			}
			ks_cond_lock(sess->monitor_cond);
			ks_cond_broadcast(sess->monitor_cond);
			ks_cond_unlock(sess->monitor_cond);
			ks_thread_join(sess->monitor_thread);
			ks_thread_destroy(&sess->monitor_thread);
		}
		sess->monitor_thread = NULL;

		swclt_conn_destroy(&sess->conn);

		// wait for all readers to finish
		ks_rwl_write_lock(sess->rwlock);

		// now destroy everything
		ks_hash_destroy(&sess->subscriptions);
		ks_hash_destroy(&sess->methods);
		ks_hash_destroy(&sess->setups);
		ks_hash_destroy(&sess->metrics);
		if (sess->ssl) {
			swclt_ssl_destroy_context(&sess->ssl);
		}
		swclt_ident_destroy(&sess->ident);
		swclt_store_destroy(&sess->store);
		ks_rwl_destroy(&sess->rwlock);
		if(sess->result_mutex) {
			flush_results(sess);
			ks_mutex_destroy(&sess->result_mutex);
		}
		ks_pool_close(&pool);
	}
	return KS_STATUS_SUCCESS;
}

static const char * __make_subscription_key(swclt_sess_t *sess, const char * protocol, const char * channel)
{
	return ks_psprintf(sess->pool, "%s:%s", protocol, channel);
}

static const char * __make_pmethod_key(swclt_sess_t *sess, const char *protocol, const char *method)
{
	return ks_psprintf(sess->pool, "%s:%s", protocol, method);
}

static swclt_pmethod_ctx_t * __make_pmethod_value(swclt_sess_t *sess, swclt_pmethod_cb_t pmethod, void *cb_data)
{
	swclt_pmethod_ctx_t *pmethod_sess = ks_pool_alloc(sess->pool, sizeof(swclt_pmethod_ctx_t));
	pmethod_sess->cb = pmethod;
	pmethod_sess->cb_data = cb_data;
	return pmethod_sess;
}

static ks_status_t __setup_ssl(swclt_sess_t *sess)
{
	swclt_ssl_destroy_context(&sess->ssl);

	return swclt_ssl_create_context(sess->config->private_key_path, sess->config->client_cert_path, sess->config->cert_chain_path, &sess->ssl);
}

static ks_bool_t __session_check_connected(swclt_sess_t *sess)
{
	return sess->state == SWCLT_STATE_ONLINE || sess->state == SWCLT_STATE_RESTORED;
}

static ks_bool_t __session_check_restored(swclt_sess_t *sess)
{
	return sess->state == SWCLT_STATE_RESTORED;
}

static ks_status_t __execute_pmethod_cb(
	swclt_sess_t *sess,
	swclt_cmd_t *cmd,
	ks_pool_t *pool,
	swclt_pmethod_ctx_t *pmethod_sess,
	const blade_execute_rqu_t *rqu)
{
	const char *err_message = NULL;
	int err_code = 0;
	char *cmd_str = swclt_cmd_describe(cmd);

	/* If the context could not be found, set the response appropriately */
	if (!pmethod_sess) {
		err_message = ks_psprintf(
					pool,
					"Failed to lookup any registered protocol method handlers for protocol: %s command: %s",
					rqu->protocol,
					cmd_str);
		err_code = -32601;
	} else {
		/* Raise the callback, if they respond with a general failure fail for them */
		ks_status_t cb_result;

		ks_log(KS_LOG_DEBUG, "Initiating execute for protocol: %s", rqu->protocol);

		if (cb_result = pmethod_sess->cb(sess, cmd, rqu, pmethod_sess->cb_data)) {
			err_message = ks_psprintf(
						pool,
						"Protocol method callback returned status: (%lu) command: %s",
						cb_result,
						cmd_str);
			err_code = -32603;
		}

		ks_log(KS_LOG_DEBUG, "Completed execute for protocol: %s (%lu)", rqu->protocol, cb_result);
	}

	/* Now verify the command was properly setup for result processing */
	if (cmd->type != SWCLT_CMD_TYPE_RESULT && cmd->type != SWCLT_CMD_TYPE_ERROR) {
		ks_debug_break();

		/* Force one for them */
		err_message = ks_psprintf(
					pool,
					"Protocol method failed to set result in command: %s",
					cmd_str);
		err_code = -32607;
	}

	/* If we errored, package up an error response */
	if (err_code) {
		ks_status_t status;

		ks_log(KS_LOG_WARNING, err_message);

		ks_json_t *err = BLADE_EXECUTE_ERR_MARSHAL(
					&(blade_execute_err_t){
						rqu->requester_nodeid,
						rqu->responder_nodeid,
						err_code,
						err_message});

		if (status = swclt_cmd_set_error(cmd, &err)) {
			ks_log(KS_LOG_WARNING, "Failed to set result in command: %s, status: %lu", cmd_str, status);
			ks_json_delete(&err);
			ks_pool_free(&err_message);
			ks_pool_free(&cmd_str);
		}

		ks_pool_free(&cmd_str);
		return status;
	}

	// All is well in the world
	return KS_STATUS_SUCCESS;
}

static ks_status_t __on_incoming_cmd(swclt_conn_t *conn, swclt_cmd_t *cmd, swclt_sess_t *sess)
{
	const char * method;
	ks_status_t status = KS_STATUS_FAIL;
	ks_json_t *request;
	ks_pool_t *cmd_pool;
	char *cmd_str;

	cmd_str = swclt_cmd_describe(cmd);

	ks_log(KS_LOG_DEBUG, "Handling incoming command: %s from transport", cmd_str);

	/* Use the commands pool for all allocations */
	cmd_pool = cmd->pool;

	/* Grab information about the method */
	method = cmd->method;
	request = cmd->json;

	/* Keep it locked until we parse the request */
	if (!strcmp(method, BLADE_BROADCAST_METHOD)) {
		/* Locate the protocol */
		blade_broadcast_rqu_t *rqu;
		swclt_sub_t *sub;
		const char *key;

		status = BLADE_BROADCAST_RQU_PARSE(cmd_pool, request, &rqu);

		if (status) {
			ks_log(KS_LOG_ERROR, "Failed to parse broadcast command: %s (%lu)",
				cmd_str, status);
			goto done;
		}

		key = __make_subscription_key(sess, rqu->protocol, rqu->channel);
		sub = ks_hash_search(sess->subscriptions, key, KS_UNLOCKED);
		ks_pool_free(&key);

		if (!sub) {
			ks_log(KS_LOG_WARNING, "Could not locate sub for protocol: %s channel: %s command: %s",
				rqu->protocol, rqu->channel, cmd_str);
			BLADE_BROADCAST_RQU_DESTROY(&rqu);
			status = KS_STATUS_NOT_FOUND;
			goto done;
		}

		status = swclt_sub_invoke(sub, sess, rqu);

		BLADE_BROADCAST_RQU_DESTROY(&rqu);
		goto done;
	} else if (!strcmp(method, BLADE_DISCONNECT_METHOD)) {
		blade_disconnect_rqu_t *rqu;

		status = BLADE_DISCONNECT_RQU_PARSE(cmd_pool, request, &rqu);

		if (status) {
			ks_log(KS_LOG_ERROR, "Failed to parse disconnect command: %s (%lu)", cmd_str, status);
			goto done;
		}

		// TODO: Handle disconnect properly, should halt sending more data until restored
		ks_json_t *cmd_result = ks_json_create_object();
		status = swclt_cmd_set_result(cmd, &cmd_result);

		BLADE_DISCONNECT_RQU_DESTROY(&rqu);

		if (!status) {
			/* Now the command is ready to be sent back, send it */
			ks_rwl_read_lock(sess->rwlock);
			status = swclt_conn_submit_result(sess->conn, cmd);
			ks_rwl_read_unlock(sess->rwlock);
			if (status)
				ks_log(KS_LOG_ERROR, "Failed to submit reply from disconnect %s, status: %lu", cmd_str, status);
			else
				ks_log(KS_LOG_DEBUG, "Sent reply back from disconnect request: %s", cmd_str);
		}
		goto done;
	} else if (!strcmp(method, BLADE_PING_METHOD)) {
		blade_ping_rqu_t *rqu;

		status = BLADE_PING_RQU_PARSE(cmd_pool, request, &rqu);

		if (status) {
			ks_log(KS_LOG_ERROR, "Failed to parse ping command: %s (%lu)", cmd_str, status);
			goto done;
		}

		ks_json_t *cmd_result = BLADE_PING_RPL_MARSHAL(&(blade_ping_rpl_t){ rqu->timestamp, rqu->payload });
		status = swclt_cmd_set_result(cmd, &cmd_result);

		BLADE_PING_RQU_DESTROY(&rqu);

		if (!status) {
			/* Now the command is ready to be sent back, send it */
			ks_rwl_read_lock(sess->rwlock);
			status = swclt_conn_submit_result(sess->conn, cmd);
			ks_rwl_read_unlock(sess->rwlock);
			if (status)
				ks_log(KS_LOG_ERROR, "Failed to submit reply from ping: %s, status: %lu", cmd_str, status);
			else
				ks_log(KS_LOG_DEBUG, "Sent reply back from ping request: %s", cmd_str);
		}
		goto done;
	} else if (!strcmp(method, BLADE_NETCAST_METHOD)) {
		blade_netcast_rqu_t *rqu;

		status = BLADE_NETCAST_RQU_PARSE(cmd_pool, request, &rqu);

		if (status) {
			ks_log(KS_LOG_ERROR, "Failed to parse netcast command: %s (%lu)", cmd_str, status);
			goto done;
		}

		if (status = swclt_store_update(sess->store, rqu)) {
			ks_log(KS_LOG_WARNING, "Failed to update nodestore from netcast command: %s (%lu)",
				cmd_str, status);
			BLADE_NETCAST_RQU_DESTROY(&rqu);
			goto done;
		}

		ks_log(KS_LOG_DEBUG, "Updated nodestore with netcast command: %s", cmd_str);
		BLADE_NETCAST_RQU_DESTROY(&rqu);
		goto done;
	} else if (!strcmp(method, BLADE_EXECUTE_METHOD)) {
		blade_execute_rqu_t *rqu;
		const char *key;

		status = BLADE_EXECUTE_RQU_PARSE(cmd_pool, request, &rqu);

		ks_log(KS_LOG_INFO, "RX: %s", cmd_str);

		if (status) {
			ks_log(KS_LOG_WARNING, "Failed to parse execute payload: %s (%lu)", cmd_str, status);
			goto done;
		}

		/* Look up the pmethod, and execute it */
		key = __make_pmethod_key(sess, rqu->protocol, rqu->method);
		if (status = __execute_pmethod_cb(sess, cmd, cmd_pool, ks_hash_search(sess->methods, key, KS_UNLOCKED), rqu)) {
			ks_log(KS_LOG_ERROR, "Error executing pmethod: %lu", status);
		}
		ks_pool_free(&key);

		/* Done with the request */
		BLADE_EXECUTE_RQU_DESTROY(&rqu);

		if (!status) {
			char *reply_str = swclt_cmd_describe(cmd);

			/* Now the command is ready to be sent back, send it */
			ks_rwl_read_lock(sess->rwlock);
			status = swclt_conn_submit_result(sess->conn, cmd);
			ks_rwl_read_unlock(sess->rwlock);
			if (status == KS_STATUS_DISCONNECTED) {
				/* send after reconnection */
				ks_log(KS_LOG_INFO, "(Not connected) TX ENQUEUE: %s", reply_str);
				enqueue_result(sess, cmd);
				status = KS_STATUS_SUCCESS;
			} else if (status) {
				ks_log(KS_LOG_ERROR, "TX FAILED %s, status: %lu", reply_str, status);
			} else {
				ks_log(KS_LOG_INFO, "TX: %s", reply_str);
			}
			ks_pool_free(&reply_str);
		}
		goto done;
	} else {
		ks_log(KS_LOG_WARNING, "Not handling incoming command: %s", cmd_str);
	}

done:

	ks_pool_free(&cmd_str);
	return status;
}

static ks_status_t __do_disconnect(swclt_sess_t *sess)
{
	ks_rwl_write_lock(sess->rwlock);
	swclt_conn_t *conn = sess->conn;
	sess->conn = NULL;
	ks_rwl_write_unlock(sess->rwlock);
	swclt_conn_destroy(&conn);
	return KS_STATUS_SUCCESS;
}

static ks_status_t __on_connect_reply(swclt_conn_t *conn, ks_json_t *error, const blade_connect_rpl_t *connect_rpl, swclt_sess_t *sess)
{
	ks_status_t status = KS_STATUS_FAIL;

	if (error && ks_json_get_object_number_int(error, "code", 0) == -32002) {
		if (sess->auth_failed_cb) sess->auth_failed_cb(sess);
	}

	if (connect_rpl) {
		status = KS_STATUS_SUCCESS;
		if (!connect_rpl->session_restored)
		{
			/* Great we got the reply populate the node store */
			swclt_store_reset(sess->store);
			if (status = swclt_store_populate(sess->store, connect_rpl)) {
				ks_log(KS_LOG_WARNING, "Failed to populate node store from connect reply (%lu)", status);
			} else {
				ks_log(KS_LOG_DEBUG, "Populated node store from connect reply");
			}
		} else {
			ks_log(KS_LOG_DEBUG, "Restored session");
		}
	}

	return status;
}

static void __on_conn_failed(swclt_conn_t *conn, void *data)
{
	swclt_sess_t *sess = (swclt_sess_t *)data;
	ks_cond_lock(sess->monitor_cond);
	sess->disconnect_time = ks_time_now_sec();
	sess->connect_time = sess->disconnect_time + 5;
	ks_cond_broadcast(sess->monitor_cond);
	ks_cond_unlock(sess->monitor_cond);
}

static ks_status_t __do_connect(swclt_sess_t *sess)
{
	ks_status_t status;
	ks_json_t *authentication = NULL;

	/* Defer this check here, so it can be rescanned from ENV at runtime after session creation */
	if (!sess->config->private_key_path || !sess->config->client_cert_path) {
		if (!sess->config->authentication) {
			ks_log(KS_LOG_ERROR, "Cannot connect without certificates or authentication");
			return KS_STATUS_FAIL;
		}
	}

	ks_log(KS_LOG_DEBUG, "Session is performing connect");

	/* Delete the previous connection if present */
	__do_disconnect(sess);

	/* Re-allocate a new ssl context */
	if (status = __setup_ssl(sess)) {
		ks_log(KS_LOG_CRIT, "SSL Setup failed: %lu", status);
		return status;
	}

	ks_log(KS_LOG_DEBUG, "Successfully setup ssl, initiating connection");

	if (sess->config->authentication) {
		authentication = ks_json_parse(sess->config->authentication);
	}

	swclt_conn_t *new_conn = NULL;

	/* Create a connection and have it call us back anytime a new read is detected */
	if (status = swclt_conn_connect_ex(
			&new_conn,
			(swclt_conn_incoming_cmd_cb_t)__on_incoming_cmd,
			sess,
			(swclt_conn_connect_cb_t)__on_connect_reply,
			sess,
			(swclt_conn_failed_cb_t)__on_conn_failed,
			sess,
			&sess->ident,
			sess->info.sessionid,			/* Pass in our session id, if it was previous valid we'll try to re-use it */
			&authentication,
			sess->config->agent,
			sess->config->identity,
			sess->config->network,
			sess->ssl)) {
		if (authentication) ks_json_delete(&authentication);
		return status;
	}

	swclt_conn_info(new_conn, &sess->info.conn);

	ks_rwl_write_lock(sess->rwlock);
	/* If we got a new session id, stash it */
	if (!ks_uuid_is_null(&sess->info.sessionid)) {
		if (ks_uuid_cmp(&sess->info.sessionid, &sess->info.conn.sessionid)) {
			ks_log(KS_LOG_WARNING, "New session id created (old: %s, new: %s), all state invalidated",
				ks_uuid_thr_str(&sess->info.sessionid), ks_uuid_thr_str(&sess->info.conn.sessionid));
			sess->state = SWCLT_STATE_ONLINE;
		} else {
			sess->state = SWCLT_STATE_RESTORED;
		}
	} else {
		sess->state = SWCLT_STATE_ONLINE;
	}

	sess->info.sessionid = sess->info.conn.sessionid;
	sess->info.nodeid = ks_pstrdup(sess->pool, sess->info.conn.nodeid);
	sess->info.master_nodeid = ks_pstrdup(sess->pool, sess->info.conn.master_nodeid);
	sess->conn = new_conn;
	ks_rwl_write_unlock(sess->rwlock);

	ks_log(KS_LOG_INFO, "Successfully established sessionid: %s, nodeid: %s  master_nodeid:%s", ks_uuid_thr_str(&sess->info.sessionid), sess->info.nodeid, sess->info.master_nodeid);

	// send any results that were enqueued during disconnect
	submit_results(sess);

	return status;
}

static void __context_describe(swclt_sess_t *sess, char *buffer, ks_size_t buffer_len)
{
	char *desc = NULL;
	if (sess->conn && (desc = swclt_conn_describe(sess->conn))) {
		snprintf(buffer, buffer_len, "SWCLT Session - %s", desc);
		ks_pool_free(&desc);
	} else {
		snprintf(buffer, buffer_len, "SWCLT Session (not connected)");
	}
}

static ks_status_t __disconnect(swclt_sess_t *sess)
{
	ks_cond_lock(sess->monitor_cond);
	sess->disconnect_time = ks_time_now_sec();
	ks_cond_broadcast(sess->monitor_cond);
	ks_cond_unlock(sess->monitor_cond);
	return KS_STATUS_SUCCESS;
}

static ks_status_t __connect(swclt_sess_t *sess)
{
	ks_cond_lock(sess->monitor_cond);
	sess->connect_time = ks_time_now_sec();
	ks_cond_broadcast(sess->monitor_cond);
	ks_cond_unlock(sess->monitor_cond);
	return KS_STATUS_SUCCESS;
}

static ks_status_t __swclt_sess_metric_register(swclt_sess_t *sess, const char *protocol, int interval, int rank)
{
	swclt_metric_reg_t *reg = NULL;

	ks_hash_write_lock(sess->metrics);

	reg = (swclt_metric_reg_t *)ks_hash_search(sess->metrics, (const void *)protocol, KS_UNLOCKED);
	if (reg) {
		ks_log(KS_LOG_DEBUG, "Metric update for '%s'\n", protocol);
		reg->interval = interval;
		reg->rank = rank;
	} else {
		ks_log(KS_LOG_DEBUG, "Metric added for '%s'\n", protocol);

		reg = (swclt_metric_reg_t *)ks_pool_alloc(ks_pool_get(sess->metrics), sizeof(swclt_metric_reg_t));
		reg->timeout = ks_time_now();
		reg->interval = interval;
		reg->rank = rank;
		reg->dirty = KS_TRUE;

		ks_hash_insert(sess->metrics, (const void *)ks_pstrdup(ks_pool_get(sess->metrics), protocol), (void *)reg);
	}

	ks_hash_write_unlock(sess->metrics);


	return KS_STATUS_SUCCESS;
}

static ks_status_t __swclt_sess_metric_update(swclt_sess_t *sess, const char *protocol, int rank)
{
	swclt_metric_reg_t *reg = NULL;

	ks_hash_write_lock(sess->metrics);

	reg = (swclt_metric_reg_t *)ks_hash_search(sess->metrics, (const void *)protocol, KS_UNLOCKED);
	if (reg && reg->rank != rank) {
		reg->rank = rank;
		reg->dirty = KS_TRUE;
	}

	ks_hash_write_unlock(sess->metrics);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __swclt_sess_metric_current(swclt_sess_t *sess, const char *protocol, int *rank)
{
	ks_status_t ret = KS_STATUS_NOT_FOUND;
	swclt_metric_reg_t *reg = NULL;

	ks_hash_read_lock(sess->metrics);

	reg = (swclt_metric_reg_t *)ks_hash_search(sess->metrics, (const void *)protocol, KS_UNLOCKED);
	if (reg) {
		*rank = reg->rank;
		ret = KS_STATUS_SUCCESS;
	}

	ks_hash_read_unlock(sess->metrics);

	return ret;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_create(
	swclt_sess_t **sessP,
	const char *identity_uri,
	swclt_config_t *config)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_pool_t *pool = NULL;
	swclt_sess_t *sess = NULL;

	ks_pool_open(&pool);
	sess = ks_pool_alloc(pool, sizeof(swclt_sess_t));
	sess->pool = pool;
	*sessP = sess;

	ks_log(KS_LOG_INFO, "Session created with identity uri: %s", identity_uri);

	/* Allow the config to be shared, no ownership change */
	sess->config = config;

	ks_rwl_create(&sess->rwlock, sess->pool);

	/* Parse the identity, it will contain the connection target address etc. */
	if (status = swclt_ident_from_str(&sess->ident, sess->pool, identity_uri)) {
		ks_log(KS_LOG_ERROR, "Invalid identity uri: %s", identity_uri);
		goto done;
	}

	/* Allocate the subscriptions hash */
	if (status = ks_hash_create(
			&sess->subscriptions,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_MUTEX,
			sess->pool))
		goto done;

	/* Allocate the methods hash */
	if (status = ks_hash_create(
			&sess->methods,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_RWLOCK,
			sess->pool))
		goto done;

	if (status = ks_hash_create(
			&sess->setups,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_RWLOCK,
			sess->pool))
		goto done;

	if (status = ks_hash_create(
			&sess->metrics,
			KS_HASH_MODE_CASE_SENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE | KS_HASH_FLAG_RWLOCK,
			sess->pool))
		goto done;

	if (status = ks_mutex_create(
			&sess->result_mutex, KS_MUTEX_FLAG_DEFAULT, sess->pool))
		goto done;

	/* Verify the config has what we need */
	if (!sess->config->private_key_path || !sess->config->client_cert_path) {
		if (!sess->config->authentication) {
			ks_log(KS_LOG_WARNING, "No authentication configured");
		}
	}

	if (status = swclt_store_create(&sess->store)) {
		ks_log(KS_LOG_ERROR, "Failed to initialize node store (%lu)", status);
		goto done;
	}

	if (status = ks_cond_create(&sess->monitor_cond, NULL)) {
		ks_log(KS_LOG_ERROR, "Failed to allocate session monitor condition: %lu", status);
		goto done;
	}

	if (status = ks_thread_create_tag(&sess->monitor_thread, session_monitor_thread, sess, NULL, "swclt-session-monitor")) {
		ks_log(KS_LOG_CRIT, "Failed to allocate session monitor thread: %lu", status);
		goto done;
	}

done:
	if (status) {
		swclt_sess_destroy(sessP);
	}

	return status;
}

static ks_status_t __nodeid_local(swclt_sess_t *sess, const char *nodeid)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_rwl_read_lock(sess->rwlock);
	status = !strcmp(sess->info.nodeid, nodeid) ? KS_STATUS_SUCCESS : KS_STATUS_FAIL;
	ks_rwl_read_unlock(sess->rwlock);
	return status;
}

static ks_status_t __unregister_subscription(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel)
{
	const char *key = __make_subscription_key(sess, protocol, channel);
	swclt_sub_t *sub = ks_hash_remove(sess->subscriptions, key);
	ks_pool_free(&key);
	if (!sub)
		return KS_STATUS_NOT_FOUND;

	swclt_sub_destroy(&sub);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __register_subscription(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_sub_t **sub)
{
	ks_rwl_read_lock(sess->rwlock);
	/* unregister if already registered so it does not leak anything, even the handle for the sub
	 * should be cleaned up to avoid leaking for the duration of the session */
	__unregister_subscription(sess, protocol, channel);

	/* And add it to the hash */
	ks_status_t status = ks_hash_insert(sess->subscriptions, __make_subscription_key(sess, protocol, channel), *sub);
	ks_rwl_read_unlock(sess->rwlock);

	if (status == KS_STATUS_SUCCESS) {
		*sub = NULL; // take ownership
	}
	return status;
}

static ks_status_t __register_pmethod(
	swclt_sess_t *sess,
	const char *protocol,
	const char *method,
	swclt_pmethod_cb_t pmethod,
	void *cb_data)
{
	if (!pmethod) {
		const char *key = __make_pmethod_key(sess, protocol, method);
		ks_hash_remove(sess->methods, key);
		ks_pool_free(&key);
		return KS_STATUS_SUCCESS;
	}
	return ks_hash_insert(sess->methods, __make_pmethod_key(sess, protocol, method), __make_pmethod_value(sess, pmethod, cb_data));
}

static ks_status_t __lookup_setup(swclt_sess_t *sess, const char *service, ks_pool_t *pool, char **protocol)
{
	ks_status_t status = KS_STATUS_NOT_FOUND;
	ks_bool_t exists = KS_FALSE;
	const char *proto = NULL;

	ks_hash_read_lock(sess->setups);
	proto = ks_hash_search(sess->setups, service, KS_UNLOCKED);
	if (proto) {
		*protocol = ks_pstrdup(pool, proto);
		status = KS_STATUS_SUCCESS;
	}
	ks_hash_read_unlock(sess->setups);

	return status;
}

static ks_status_t __register_setup(swclt_sess_t *sess, const char *service, const char *protocol)
{
	ks_status_t status;

	ks_hash_write_lock(sess->setups);
	status = ks_hash_insert(sess->setups, ks_pstrdup(ks_pool_get(sess->setups), service), ks_pstrdup(ks_pool_get(sess->setups), protocol));
	ks_hash_write_unlock(sess->setups);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_set_auth_failed_cb(swclt_sess_t *sess, swclt_sess_auth_failed_cb_t cb)
{
	sess->auth_failed_cb = cb;
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_set_state_change_cb(swclt_sess_t *sess, swclt_sess_state_change_cb_t cb, void *cb_data)
{
	sess->state_change_cb = cb;
	sess->state_change_cb_data = cb_data;
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_target_set(swclt_sess_t *sess, const char *target)
{
	ks_status_t result = KS_STATUS_SUCCESS;

	if (!sess) {
		return result;
	}

	/* Parse the identity, it will contain the connection target address etc. */
	if (result = swclt_ident_from_str(&sess->ident, sess->pool, target)) {
		ks_log(KS_LOG_ERROR, "Invalid identity uri: %s", target);
		goto done;
	}
	ks_log(KS_LOG_ERROR, "Updated session target to %s", target);

done:

	return result;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_register(swclt_sess_t *sess, const char *protocol, int interval, int rank)
{
	if (!protocol) {
		ks_log(KS_LOG_ERROR, "Missing protocol for rank register");
		return KS_STATUS_ARG_INVALID;
	}
	if (interval <= 0) {
		ks_log(KS_LOG_ERROR, "Invalid rank interval %d for protocol %s", interval, protocol);
		return KS_STATUS_ARG_INVALID;
	}
	if (rank < 0) {
		ks_log(KS_LOG_ERROR, "Invalid rank %d for protocol %s", rank, protocol);
		return KS_STATUS_ARG_INVALID;
	}
	return __swclt_sess_metric_register(sess, protocol, interval, rank);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_update(swclt_sess_t *sess, const char *protocol, int rank)
{
	if (!protocol) {
		ks_log(KS_LOG_ERROR, "Missing protocol for rank update");
		return KS_STATUS_ARG_INVALID;
	}
	if (rank < 0) {
		ks_log(KS_LOG_ERROR, "Invalid rank %d for protocol %s", rank, protocol);
		return KS_STATUS_ARG_INVALID;
	}
	return __swclt_sess_metric_update(sess, protocol, rank);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_current(swclt_sess_t *sess, const char *protocol, int *rank)
{
	if (!protocol) {
		ks_log(KS_LOG_ERROR, "Missing protocol for rank update");
		return KS_STATUS_ARG_INVALID;
	}
	return __swclt_sess_metric_current(sess, protocol, rank);
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_has_authentication(swclt_sess_t *sess)
{
	return (sess->config->private_key_path && sess->config->client_cert_path) || sess->config->authentication;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_connect(swclt_sess_t *sess)
{
	return __connect(sess);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_disconnect(swclt_sess_t *sess)
{
	return __disconnect(sess);
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_connected(swclt_sess_t *sess)
{
	ks_bool_t connected;
	ks_rwl_read_lock(sess->rwlock);
	connected = __session_check_connected(sess);
	ks_rwl_read_unlock(sess->rwlock);
	return connected;
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_restored(swclt_sess_t *sess)
{
	ks_bool_t connected;
	ks_rwl_read_lock(sess->rwlock);
	connected = __session_check_restored(sess);
	ks_rwl_read_unlock(sess->rwlock);
	return connected;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_info(
	swclt_sess_t *sess,
	ks_pool_t *pool,
	ks_uuid_t *sessionid,
	char **nodeid,
	char **master_nodeid)
{
	ks_rwl_read_lock(sess->rwlock);
	if (__session_check_connected(sess)) {
		/* Default to our pool if no pool specified */
		if (!pool)
			pool = sess->pool;

		/* Context will be locked now, safe the access info */
		if (sessionid)
			*sessionid = sess->info.sessionid;
		if (nodeid)
			*nodeid = ks_pstrdup(pool, sess->info.nodeid);
		if (master_nodeid)
			*master_nodeid = ks_pstrdup(pool, sess->info.master_nodeid);
	}
	ks_rwl_read_unlock(sess->rwlock);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_nodeid(swclt_sess_t *sess, ks_pool_t *pool, char **nodeid)
{
	ks_rwl_read_lock(sess->rwlock);
	*nodeid = ks_pstrdup(pool, sess->info.nodeid);
	ks_rwl_read_unlock(sess->rwlock);
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_bool_t) swclt_sess_nodeid_local(swclt_sess_t *sess, const char *nodeid)
{
	return swclt_sess_connected(sess) && !__nodeid_local(sess, nodeid);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_register_protocol_method(
	swclt_sess_t *sess,
	const char *protocol,
	const char *method,
	swclt_pmethod_cb_t pmethod_cb,
	void *cb_data)
{
	return __register_pmethod(sess, protocol, method, pmethod_cb, cb_data);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_register_subscription_method(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_sub_t *sub = NULL;
	if (status = swclt_sub_create(&sub, sess->pool, protocol, channel, cb, cb_data))
		return status;

	/* Register this subscription, if request fails we just won't receive the events,
	 * but registering the callback is fine and can be replaced if client tries again */
	status = __register_subscription(sess, protocol, channel, &sub);
	swclt_sub_destroy(&sub);
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_broadcast(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	const char *event,
	ks_json_t **params)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_BROADCAST_CMD(
			protocol,
			channel,
			event,
			sess->info.nodeid,
			params))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* Now submit it */
	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, NULL);
	ks_rwl_read_unlock(sess->rwlock);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_subscription_add_async(
		sess,
		protocol,
		channel,
		cb,
		cb_data,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add_async(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;
	blade_subscription_rpl_t *subscription_rpl = NULL;
	swclt_sub_t *sub = NULL;

	/* @todo remove the next 2 calls, and call swclt_sess_register_subscription_method externally, and
	 * update to allow multiple channels to be subscribed via ks_json_t array of channel name strings
	 * can also verify that the callbacks are already registered here as a sanity check */

	/* We also will track this subscription with a handle */
	if (status = swclt_sub_create(&sub, sess->pool, protocol, channel, cb, cb_data))
		goto done;

	/* Register this subscription, if request fails we just won't receive the events,
	 * but registering the callback is fine and can be replaced if client tries again */
	if (status = __register_subscription(sess, protocol, channel, &sub))
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
	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

done:
	swclt_cmd_destroy(&cmd);
	swclt_sub_destroy(&sub);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_subscription_remove_async(
		sess,
		protocol,
		channel,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove_async(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

	/* Unregister this subscription if it exists, if not then just continue with subscription removal
	 * because the callback may have been removed manually */
	__unregister_subscription(sess, protocol, channel);

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
	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add(
	swclt_sess_t *sess,
	const char * protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
	ks_json_t **channels,
	int rank,
	ks_json_t **data,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_protocol_provider_add_async(
		sess,
		protocol,
		default_method_execute_access,
		default_channel_subscribe_access,
		default_channel_broadcast_access,
		methods,
		channels,
		rank,
		data,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add_async(
	swclt_sess_t *sess,
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
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

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

	/* If the caller wants to do async, set the callback in the cmd */
	if (response_callback) {
		if (status = swclt_cmd_set_cb(cmd, response_callback, response_callback_data))
			goto done;
	}

	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove(
	swclt_sess_t *sess,
	const char * protocol,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_protocol_provider_remove_async(
		sess,
		protocol,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove_async(
	swclt_sess_t *sess,
	const char * protocol,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

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

	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update(
	swclt_sess_t *sess,
	const char * protocol,
	int rank,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_protocol_provider_rank_update_async(
		sess,
		protocol,
		rank,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update_async(
	swclt_sess_t *sess,
	const char * protocol,
	int rank,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

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

	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}


SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add(
	swclt_sess_t *sess,
	const char *identity,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_identity_add_async(
		sess,
		identity,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add_async(
	swclt_sess_t *sess,
	const char *identity,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

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

	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_wait_for_cmd_reply(
	swclt_sess_t *sess,
	swclt_cmd_future_t **future,
	swclt_cmd_reply_t **reply)
{
	ks_status_t status = KS_STATUS_FAIL;
	if (future && *future) {
		status = swclt_cmd_future_get(*future, reply);
		if (status != KS_STATUS_SUCCESS) {
			ks_rwl_read_lock(sess->rwlock);
			if (sess->conn) {
				swclt_conn_cancel_request(sess->conn, future);
			}
			ks_rwl_read_unlock(sess->rwlock);
		}
		swclt_cmd_future_destroy(future);
	}
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_execute(
	swclt_sess_t *sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_execute_async(
		sess,
		responder,
		protocol,
		method,
		params,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_with_id(
	swclt_sess_t *sess,
	const char *id,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_execute_with_id_async(
		sess,
		id,
		responder,
		protocol,
		method,
		params,
		NULL,
		NULL,
		&future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_async(
	swclt_sess_t *sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_EXECUTE_CMD(
			NULL,
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

	char *request_str = strdup(swclt_cmd_describe(cmd));

	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

	if (status) {
		ks_log(KS_LOG_WARNING, "FAILED TX: %s", request_str);
	} else {
		ks_log(KS_LOG_INFO, "TX: %s", request_str);
	}
	free(request_str);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_with_id_async(
	swclt_sess_t *sess,
	const char *id,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	swclt_cmd_t *cmd = NULL;

	/* Create the command */
	if (!(cmd = CREATE_BLADE_EXECUTE_CMD(
			id,
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

	char *request_str = strdup(swclt_cmd_describe(cmd));

	ks_rwl_read_lock(sess->rwlock);
	status = swclt_conn_submit_request(sess->conn, &cmd, future);
	ks_rwl_read_unlock(sess->rwlock);

	if (status) {
		ks_log(KS_LOG_WARNING, "FAILED TX: %s", request_str);
	} else {
		ks_log(KS_LOG_INFO, "TX: %s", request_str);
	}
	free(request_str);

done:
	swclt_cmd_destroy(&cmd);

	return status;
}

// signalwire consumer

SWCLT_DECLARE(ks_status_t) swclt_sess_signalwire_setup(swclt_sess_t *sess, const char *service, swclt_sub_cb_t cb, void *cb_data)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	swclt_store_t *store;
	ks_json_t *params = NULL;
	swclt_cmd_reply_t *reply = NULL;
	ks_json_t *result = NULL;
	const char *protocol = NULL;
	ks_bool_t instance_found = KS_FALSE;

	if (!service) {
		ks_log(KS_LOG_ERROR, "Missing service for signalwire.setup");
		return KS_STATUS_ARG_INVALID;
	}

	// Make sure we are at least connected
	if (!swclt_sess_connected(sess)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' failed because session is not connected", service);
		status = KS_STATUS_INACTIVE;
		goto done;
	}

	params = ks_json_create_object();
	ks_json_add_string_to_object(params, "service", service);

	// Send the setup request syncronously, if it fails bail out
	if (status = swclt_sess_execute(sess,
									NULL,
									"signalwire",
									"setup",
									&params,
									&reply)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' execute failed: %d", service, status);
		goto done;
	}

	// Get protocol from result, duplicate it so we can destroy the command
	protocol = ks_json_get_object_string(ks_json_get_object_item(reply->json, "result"), "protocol", NULL);
	if (protocol) protocol = ks_pstrdup(sess->pool, protocol);

	if (!protocol) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' response has no result.result.protocol", service);
		status = KS_STATUS_ARG_NULL;
		goto done;
	}

	// Destroy the reply, taking the result data with it, protocol is duplicated in session pool
	swclt_cmd_reply_destroy(&reply);

	ks_log(KS_LOG_DEBUG, "Setup for '%s' waiting for provider of protocol instance: %s", service, protocol);

	// poll nodestore until protocol is seen locally (or timeout after 2 seconds of waiting), which
	// ensures our upstream also sees it which means we can subscribe to the channel without failing
	// if it's not known by upstream yet
	{
		int nodestore_attempts = 20;
		while (!instance_found && nodestore_attempts) {
			if (!(instance_found = !swclt_store_check_protocol(sess->store, protocol))) {
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
	if (status = swclt_sess_subscription_add(sess, protocol, "notifications", cb, cb_data, NULL)) {
		ks_log(KS_LOG_ERROR, "Setup for '%s' subscription add failed: %d", service, status);
		goto done;
	}
	__register_setup(sess, service, protocol);

done:
	if (protocol) ks_pool_free(&protocol);
	swclt_cmd_reply_destroy(&reply);
	if (params) ks_json_delete(&params);

	return status;
}

// signalwire provisioning consumer

SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_setup(swclt_sess_t *sess, swclt_sub_cb_t cb, void *cb_data)
{
	return swclt_sess_signalwire_setup(sess, "provisioning", cb, cb_data);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure(swclt_sess_t *sess,
															 const char *target,
															 const char *local_endpoint,
															 const char *external_endpoint,
															 const char *relay_connector_id,
															 swclt_cmd_reply_t **reply)
{
	swclt_cmd_future_t *future = NULL;
	swclt_sess_provisioning_configure_async(sess,
												   target,
												   local_endpoint,
												   external_endpoint,
												   relay_connector_id,
												   NULL,
												   NULL,
												   &future);
	return swclt_sess_wait_for_cmd_reply(sess, &future, reply);
}

SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure_async(swclt_sess_t *sess,
																   const char *target,
																   const char *local_endpoint,
																   const char *external_endpoint,
																   const char *relay_connector_id,
																   swclt_cmd_cb_t response_callback,
																   void *response_callback_data,
																	swclt_cmd_future_t **future)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	swclt_store_t *store;
	ks_pool_t *pool = NULL;
	char *protocol = NULL;
	ks_json_t *params = NULL;

	if (!target || !local_endpoint || !external_endpoint || !relay_connector_id) {
		ks_log(KS_LOG_ERROR, "Missing required parameter");
		return KS_STATUS_ARG_INVALID;
	}

	if (!swclt_sess_connected(sess)) {
		goto done;
	}

	pool = sess->pool;

	if (__lookup_setup(sess, "provisioning", pool, &protocol)) {
		ks_log(KS_LOG_ERROR, "Provisioning setup has not been performed");
		status = KS_STATUS_FAIL;
		goto done;
	}

	params = ks_json_create_object();

	ks_json_add_string_to_object(params, "target", target);
	ks_json_add_string_to_object(params, "local_endpoint", local_endpoint);
	ks_json_add_string_to_object(params, "external_endpoint", external_endpoint);
	ks_json_add_string_to_object(params, "relay_connector_id", relay_connector_id);

	status = swclt_sess_execute_async(sess,
									  NULL,
									  protocol,
									  "configure",
									  &params,
									  response_callback,
									  response_callback_data,
									  future);
done:
	if (protocol) ks_pool_free(&protocol);
	if (params) ks_json_delete(&params);

	return status;
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