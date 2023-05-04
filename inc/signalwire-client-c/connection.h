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

#pragma once

KS_BEGIN_EXTERN_C

typedef struct swclt_ttl_tracker swclt_ttl_tracker_t;

typedef struct swclt_conn swclt_conn_t;

typedef ks_status_t (*swclt_conn_incoming_cmd_cb_t)(swclt_conn_t *conn, swclt_cmd_t *cmd, void *cb_data);
typedef ks_status_t (*swclt_conn_connect_cb_t)(swclt_conn_t *conn, ks_json_t *error, blade_connect_rpl_t *connect_rpl, void *cb_data);
typedef ks_status_t (*swclt_conn_failed_cb_t)(swclt_conn_t *conn, void *cb_data);

/* Information about this connection */
typedef struct swclt_conn_info_s {
	/* We also store a copy of the wss's info structure */
	swclt_wss_info_t wss;

	/* Pulled from the blade connect result */
	ks_uuid_t sessionid;
	const char *nodeid;
	const char *master_nodeid;
} swclt_conn_info_t;

/* Ths client connection context represents a connections state */
struct swclt_conn {

	ks_pool_t *pool;

	/* When we receive an incoming request we call this callback with the prepared command */
	swclt_conn_incoming_cmd_cb_t incoming_cmd_cb;
	void *incoming_cmd_cb_data;

	/* Optional callbacks for getting the initial connect result payload */
	swclt_conn_connect_cb_t connect_cb;
	void *connect_cb_data;

	/* Optional callbacks for getting notified of connection failure */
	swclt_conn_failed_cb_t failed_cb;
	void *failed_cb_data;

	/* Connection failed state */
	int failed;

	ks_mutex_t *failed_mutex;

	/* Our websocket transport, basically our connection to blade */
	swclt_wss_t *wss;

	/* Basic connection info that the caller can examine, contains
	 * our sessionid, nodeid, and master_nodeid variables returned from
	 * a connect result from blade, including our connected address and
	 * ssl context ptr (from websocket info) */
	swclt_conn_info_t info;

	/* The result of our last connect, kept around for reference */
	blade_connect_rpl_t *blade_connect_rpl;

	/* A hash of outstanding commands, keyed by their request ids.
	 * This is the outgoing queue for requests born from the client or
	 * requests which have been sent from blade. Since the uuids are
	 * globally unique we can just use one hash for both */
	ks_hash_t *outstanding_requests;

	/* TTLs to expire */
	swclt_ttl_tracker_t *ttl;

	/* pool to process incoming websocket frames */
	ks_thread_pool_t *incoming_frame_pool;

	/* when last stats were published */
	ks_time_t last_stats_update;
	swclt_wss_stats_t last_stats;
};

SWCLT_DECLARE(void) swclt_conn_destroy(swclt_conn_t **conn);
SWCLT_DECLARE(ks_status_t) swclt_conn_connect(
	swclt_conn_t **conn,
	swclt_conn_incoming_cmd_cb_t incoming_command_callback,
	void *incoming_command_cb_data,
	swclt_ident_t *ident,
	ks_json_t **authentication,
	const char *agent,
	const char *identity,
	ks_json_t *network,
	const SSL_CTX *ssl);

SWCLT_DECLARE(ks_status_t) swclt_conn_connect_ex(
	swclt_conn_t **conn,
	swclt_conn_incoming_cmd_cb_t incoming_command_callback,
	void *incoming_command_cb_data,
	swclt_conn_connect_cb_t connect_callback,
	void *connect_cb_data,
	swclt_conn_failed_cb_t failed_callback,
	void *failed_cb_data,
	swclt_ident_t *ident,
	ks_uuid_t previous_sessionid,
	ks_json_t **authentication,
	const char *agent,
	const char *identity,
	ks_json_t *network,
	const SSL_CTX *ssl);

SWCLT_DECLARE(ks_status_t) swclt_conn_submit_request(swclt_conn_t *conn, swclt_cmd_t **cmd, swclt_cmd_future_t **future);
SWCLT_DECLARE(ks_status_t) swclt_conn_submit_result(swclt_conn_t *conn, swclt_cmd_t *cmd);
SWCLT_DECLARE(ks_status_t) swclt_conn_cancel_request(swclt_conn_t *conn, swclt_cmd_future_t **future);
ks_status_t swclt_conn_info(swclt_conn_t *conn, swclt_conn_info_t *info);
SWCLT_DECLARE(char *) swclt_conn_describe(swclt_conn_t *conn);

KS_END_EXTERN_C

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
