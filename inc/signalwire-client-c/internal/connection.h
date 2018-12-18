/*
 * Copyright (c) 2018 SignalWire, Inc
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
struct swclt_conn_ctx {
	swclt_handle_base_t base;

	/* When we receive an incoming request we call this callback with the prepared command */
	swclt_conn_incoming_cmd_cb_t incoming_cmd_cb;
	void *incoming_cmd_cb_data;

	/* Optional callbacks for getting the initial connect result payload */
	swclt_conn_connect_cb_t connect_cb;
	void *connect_cb_data;

	/* Our websocket transport, basically our connection to blade */
	swclt_wss_t wss;

	/* Basic connection info that the caller can examine, contains
	 * our sessionid, nodeid, and master_nodeid variables returned from
	 * a connect result from blade, including our connected address and
	 * ssl context ptr (from websocket info) */
	swclt_conn_info_t info;

	/* The result of our last connect, kept around for reference */
	blade_connect_rpl_t *blade_connect_rpl;

	/* A hash of oustanding commands, keyed by their request ids.
	 * This is the outgoing queue for requests born from the client or
	 * requests which have been sent from blade. Since the uuids are
	 * globally unique we can just use one hash for both */
	ks_hash_t *outstanding_requests;

	/* The outstanding condition is signalled anytime a command in the outstanding
	 * requests hash parses a result. Client readers wait on this condition until
	 * signalled */
	ks_cond_t *cmd_condition;
};

/* Private */
ks_status_t swclt_conn_info(swclt_conn_t conn, swclt_conn_info_t *info);

KS_END_EXTERN_C

/* Define helper macros to eliminate boiler code in handle wrapped apis */
#define SWCLT_CONN_SCOPE_BEG(conn, ctx, status) \
	KS_HANDLE_SCOPE_BEG(SWCLT_HTYPE_CONN, conn, swclt_conn_ctx_t, ctx, status);

#define SWCLT_CONN_SCOPE_END(conn, ctx, status) \
	KS_HANDLE_SCOPE_END(SWCLT_HTYPE_CONN, conn, swclt_conn_ctx_t, ctx, status);

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
