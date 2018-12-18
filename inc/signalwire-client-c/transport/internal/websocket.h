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

struct swclt_wss_ctx {
	swclt_handle_base_t base;

	/* Callback for when the reader reads a new frame */
	swclt_wss_incoming_frame_cb_t incoming_frame_cb;
	void *incoming_frame_cb_data;

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
	swclt_frame_t read_frame;

	/* Our final status for our read thread, and its thread context
	 * for thread control (e.g. stop requests) */
	ks_status_t reader_status;
	ks_thread_t *reader_thread;

	/* Build in rate calculators track throughput for send/recv all the time */
	ks_throughput_t rate_send, rate_recv;

	/* To maintain state, we lock with mutexes. We have two here
	 * the read_mutex is locked anytime someone is reading from
	 * the websocket, and the write_mutex is locked anytime
	 * someone is writing to the websocket */
	ks_mutex_t *read_mutex, *write_mutex;
};

KS_END_EXTERN_C

/* Define helper macros to eliminate boiler code in handle wrapped apis */
#define SWCLT_WSS_SCOPE_BEG(wss, ctx, status) \
	KS_HANDLE_SCOPE_BEG(SWCLT_HTYPE_WSS, wss, swclt_wss_ctx_t, ctx, status);

#define SWCLT_WSS_SCOPE_END(wss, ctx, status) \
	KS_HANDLE_SCOPE_END(SWCLT_HTYPE_WSS, wss, swclt_wss_ctx_t, ctx, status);

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
