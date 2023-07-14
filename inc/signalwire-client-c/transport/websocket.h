/*
 * Copyright (c) 2018-2020 SignalWire, Inc
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

typedef struct swclt_wss swclt_wss_t;

typedef ks_status_t (*swclt_wss_incoming_frame_cb_t)(swclt_wss_t *wss, swclt_frame_t **frame, void *cb_data);
typedef ks_status_t (*swclt_wss_failed_cb_t)(swclt_wss_t *wss, void *cb_data);

/* Define our info structure */
typedef struct swclt_wss_info_s {
	char address[128];				/* tcp address string (no port) */
	char cipher[128];				/* negotiated ssl cipher name */
	uint16_t port;					/* port we are connected to */
	char path[128];				    /* endpoint path */
	uint32_t connect_timeout_ms;	/* our connect timeout */
	SSL_CTX *ssl;					/* ssl context handed to us at first connect attempt */
} swclt_wss_info_t;

typedef struct swclt_wss_stats {
	int64_t read_frames;
	int64_t write_frames;
} swclt_wss_stats_t;

struct swclt_wss {

	ks_pool_t *pool;

	int failed;

	/* Callback for when the reader reads a new frame */
	swclt_wss_incoming_frame_cb_t incoming_frame_cb;
	void *incoming_frame_cb_data;

	/* Callback for when the websocket fails */
	swclt_wss_failed_cb_t failed_cb;
	void *failed_cb_data;

	/* Information concerning our connection */
	swclt_wss_info_t info;

	/* Raw socket from libks */
	ks_socket_t socket;

	/* Resolved address structure */
	ks_sockaddr_t addr;				

	/* Web socket from libks */
	kws_t *wss;

	/* Ping management */
	ks_time_t ping_next_time_sec;

	/* We keep a read frame around and re-use its data buffer */
	swclt_frame_t *read_frame;

	/* Our final status for our read thread, and its thread context
	 * for thread control (e.g. stop requests) */
	ks_status_t reader_status;
	ks_thread_t *reader_thread;

	ks_mutex_t *wss_mutex;
	swclt_wss_stats_t stats;
};

SWCLT_DECLARE(ks_status_t) swclt_wss_connect(
	swclt_wss_t **wss,
	swclt_wss_incoming_frame_cb_t incoming_frame_cb,
	void *incoming_frame_cb_data,
	swclt_wss_failed_cb_t failed_cb,
	void *failed_cb_data,
	const char *address,
	short port,
	const char *path,
	uint32_t timeout_ms,
	const SSL_CTX *ssl);

SWCLT_DECLARE(void) swclt_wss_destroy(swclt_wss_t **wss);

SWCLT_DECLARE(ks_status_t) swclt_wss_write(swclt_wss_t *wss, char *data);
SWCLT_DECLARE(ks_status_t) swclt_wss_get_info(swclt_wss_t *wss, swclt_wss_info_t *info);
SWCLT_DECLARE(char *) swclt_wss_describe(swclt_wss_t *ctx);
SWCLT_DECLARE(void) swclt_wss_get_stats(swclt_wss_t *ctx, swclt_wss_stats_t *stats);

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
