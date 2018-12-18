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

/* Obfuscate our connection internals */
typedef struct swclt_wss_ctx swclt_wss_ctx_t;

typedef ks_handle_t swclt_wss_t;

/* Define our info structure */
typedef struct swclt_wss_info_s {
	char address[128];				/* tcp address string (no port) */
	char cipher[128];				/* negotiated ssl cipher name */
	uint16_t port;					/* port we are connected to */
	char path[128];				    /* endpoint path */
	uint32_t connect_timeout_ms;	/* our connect timeout */
	SSL_CTX *ssl;					/* ssl context handed to us at first connect attempt */
} swclt_wss_info_t;

typedef ks_status_t (*swclt_wss_incoming_frame_cb_t)(swclt_wss_t wss, swclt_frame_t frame, void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_wss_connect(
	swclt_wss_t *wss,
	swclt_wss_incoming_frame_cb_t incoming_frame_cb,
	void *incoming_frame_cb_data,
	const char *address,
	short port,
	const char *path,
	uint32_t timeout_ms,
	const SSL_CTX *ssl);

SWCLT_DECLARE(ks_status_t) swclt_wss_write_cmd(swclt_wss_t wss, swclt_cmd_t cmd);
SWCLT_DECLARE(ks_status_t) swclt_wss_get_rates(swclt_wss_t wss, ks_throughput_t *send, ks_throughput_t *recv);
SWCLT_DECLARE(ks_status_t) swclt_wss_get_info(swclt_wss_t wss, swclt_wss_info_t *info);

#define swclt_wss_get(wss, contextP)		ks_handle_get(SWCLT_HTYPE_WSS, wss, contextP)
#define swclt_wss_put(contextP)				ks_handle_put(SWCLT_HTYPE_WSS, contextP)

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
