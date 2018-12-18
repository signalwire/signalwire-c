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

/* All connections are handles, opaque numbers that manage ref counts */
typedef ks_handle_t swclt_conn_t;

/* Obfuscate our connection internals */
typedef struct swclt_conn_ctx swclt_conn_ctx_t;

typedef ks_status_t (*swclt_conn_incoming_cmd_cb_t)(swclt_conn_t conn, swclt_cmd_t cmd, void *cb_data);
typedef ks_status_t (*swclt_conn_connect_cb_t)(swclt_conn_t conn, ks_json_t *error, blade_connect_rpl_t *connect_rpl, void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_conn_connect(
	swclt_conn_t *conn,
	swclt_conn_incoming_cmd_cb_t incoming_command_callback,
	void *incoming_command_cb_data,
	swclt_ident_t *ident,
	ks_json_t **authentication,
	const SSL_CTX *ssl);

SWCLT_DECLARE(ks_status_t) swclt_conn_connect_ex(
	swclt_conn_t *conn,
	swclt_conn_incoming_cmd_cb_t incoming_command_callback,
	void *incoming_command_cb_data,
	swclt_conn_connect_cb_t connect_callback,
	void *connect_cb_data,
	swclt_ident_t *ident,
	ks_uuid_t previous_sessionid,
	ks_json_t **authentication,
	const SSL_CTX *ssl);

SWCLT_DECLARE(ks_status_t) swclt_conn_submit_request(swclt_conn_t conn, swclt_cmd_t cmd);
SWCLT_DECLARE(ks_status_t) swclt_conn_submit_result(swclt_conn_t conn, swclt_cmd_t cmd);
SWCLT_DECLARE(ks_status_t) swclt_conn_get_rates(swclt_conn_t conn, ks_throughput_t *recv, ks_throughput_t *send);

#define swclt_conn_get(conn, contextP)		__ks_handle_get(SWCLT_HTYPE_CONN, conn, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_conn_put(contextP)			__ks_handle_put(SWCLT_HTYPE_CONN, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)

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
