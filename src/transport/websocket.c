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

#include "signalwire-client-c/client.h"
#include "signalwire-client-c/transport/internal/websocket.h"
#include "signalwire-client-c/transport/internal/frame.h"

// Write apis

static ks_status_t __write_raw(swclt_wss_ctx_t *ctx, kws_opcode_t opcode, const void *data, ks_size_t len)
{
	ks_size_t wrote;

	ks_log(KS_LOG_DEBUG, "Writing frame of size: %lu opcode: %lu\n", len, opcode);

	ks_mutex_lock(ctx->write_mutex);
	wrote = kws_write_frame(ctx->wss, opcode, data, len);
	ks_mutex_unlock(ctx->write_mutex);

	if (wrote != len) {
		ks_log(KS_LOG_WARNING, "Failed to write frame\n");
		return KS_STATUS_FAIL;
	} else {
		ks_throughput_report_ex(ctx->rate_send, len, KS_FALSE);
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __write_ping(swclt_wss_ctx_t *ctx)
{
	static uint64_t data = 0;
	return __write_raw(ctx, WSOC_PING, &data, sizeof(data));
}

static ks_status_t __write_pong(swclt_wss_ctx_t *ctx, swclt_frame_t frame)
{
	swclt_frame_ctx_t *frame_ctx;
	ks_status_t status = KS_STATUS_SUCCESS;

	/* Reference the frame while we write it */
	if (status = swclt_frame_get(frame, &frame_ctx)) {
		ks_log(KS_LOG_WARNING, "Invalid frame handed to write: %16.16llx (%lu)", frame, status);
		return status;
	}

	ks_mutex_lock(ctx->write_mutex);
	if (kws_write_frame(ctx->wss, WSOC_PONG, frame_ctx->data, frame_ctx->len) != frame_ctx->len) {
		ks_log(KS_LOG_WARNING, "Failed to write frame: %s", ks_handle_describe(frame));
		status = KS_STATUS_FAIL;
	}
	ks_mutex_unlock(ctx->write_mutex);

	/* Done with the reference */
	swclt_frame_put(&frame_ctx);

	return status;
}

// Read apis

static ks_status_t __poll_read(swclt_wss_ctx_t *ctx, uint32_t *poll_flagsP)
{
	*poll_flagsP = ks_wait_sock(ctx->socket, 1000, KS_POLL_READ);
	if (*poll_flagsP & KS_POLL_ERROR) {
		return KS_STATUS_FAIL;
	}

	return KS_STATUS_SUCCESS;
}

// Read thread
static ks_status_t __read_frame(swclt_wss_ctx_t *ctx, swclt_frame_t *frameP, kws_opcode_t *opcodeP)
{
	swclt_frame_t frame = KS_NULL_HANDLE;
	kws_opcode_t opcode = WSOC_INVALID;
	uint8_t *data;
	ks_ssize_t len;
	ks_status_t status;

	/* Allocate a new frame if needed */
	if (!*frameP) {
		if (status = swclt_frame_alloc(&frame))
			return status;

		/* Associate the frame with our handle */
		ks_handle_set_parent(frame, ctx->base.handle);
	} else {
		frame = *frameP;

		/* Mark it null for now if we error its getting freed */
		*frameP = KS_NULL_HANDLE;
	}

	ks_mutex_lock(ctx->write_mutex);

	/* kws will actually retain ownership of its buffer so, we'll want to make a copy
	 * into our frame */
	len = kws_read_frame(ctx->wss, &opcode, &data);

	if (len < 0) {
		ks_log(KS_LOG_WARNING, "Read frame failed");
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* Update rates */
	ks_throughput_report_ex(ctx->rate_recv, (size_t)len, KS_FALSE);

	ks_log(KS_LOG_DEBUG, "Copying frame of length: %lu of opcode: %lu", (size_t)len, opcode);

	/* Stash it in the frame */
	if (status = swclt_frame_copy_data(frame, data, (size_t)len, opcode))
		goto done;

done:
	ks_mutex_unlock(ctx->write_mutex);

	if (status) {
		ks_handle_destroy(&frame);
	} else {
		*frameP = frame;
		*opcodeP = opcode;
	}

	return status;
}
static ks_status_t __reader_loop(swclt_wss_ctx_t *ctx)
{
	ks_status_t status;

	/* Righto first things first, notify the throughputs that
	 * their clocks can officially start. */
	ks_throughput_start(ctx->rate_recv);
	ks_throughput_start(ctx->rate_send);

	ks_log(KS_LOG_DEBUG, "Websocket reader starting");

	/* Loop until the caller requests a stop */
	while (!ks_thread_stop_requested(ctx->reader_thread)) {
		uint32_t poll_flags;
		kws_opcode_t frame_opcode;

		if (status = swclt_hstate_check_ctx(&ctx->base, "Reader stopping due to state change:"))
			return status;

		ks_log(KS_LOG_DEBUG, "Waiting on read poll");

		if (status = __poll_read(ctx, &poll_flags)) {
			ks_log(KS_LOG_WARNING, "Poll failed: %d", status);
			return status;
		}

		ks_log(KS_LOG_DEBUG, "Woke up from read poll, flags: %d", poll_flags);

		if (poll_flags & KS_POLL_READ) {
			ks_log(KS_LOG_DEBUG, "Reading new frame");

			if (status = __read_frame(ctx, &ctx->read_frame, &frame_opcode)) {
				ks_log(KS_LOG_WARNING, "Failed to read frame: %d", status);
				return status;
			}

			ks_log(KS_LOG_DEBUG, "Read frame: %s", ks_handle_describe(ctx->read_frame));

			// Deal with pings
			switch(frame_opcode) {
			case WSOC_PING:
				ks_log(KS_LOG_DEBUG, "PING");
				if (status = __write_pong(ctx, ctx->read_frame)) {
					ks_log(KS_LOG_WARNING, "Failed to write ping: %d", status);
					return status;
				}
				break;

			case WSOC_PONG:
				ks_log(KS_LOG_DEBUG, "PONG");
				break;

			case WSOC_TEXT:
				ks_log(KS_LOG_DEBUG, "Reading text ");
				// Notify the consumer there is a new frame
				if (status = ctx->incoming_frame_cb(ctx->base.handle, ctx->read_frame, ctx->incoming_frame_cb_data)) {
					ks_log(KS_LOG_WARNING, "Callback from incoming frame returned: %d, exiting", status);
					// Done with the frame, callback is responsible for freeing it
					ctx->read_frame = 0;
					return status;
				}

				// Done with the frame, callback is responsible for freeing it
				ctx->read_frame = 0;
				break;
			}
		} else {
			/* See if we're due */
			if (ks_time_now_sec() >= ctx->ping_next_time_sec) {
				ks_log(KS_LOG_DEBUG, "PING");
				if (status = __write_ping(ctx)) {
					ks_log(KS_LOG_WARNING, "Failed to write: %d", status);
					return status;
				}

				/* Ping again in 10 seconds */
				ctx->ping_next_time_sec = ks_time_now_sec() + 10;
			} else {
				ks_sleep_ms(250);
			}
		}
	}

	ks_log(KS_LOG_INFO, "Websocket reader exiting due to stop request");
	return KS_STATUS_THREAD_STOP_REQUESTED;
}

static void *__reader(ks_thread_t *thread, void *data)
{
	swclt_wss_ctx_t *ctx = (swclt_wss_ctx_t *)data;
	ctx->reader_status = __reader_loop(ctx);

	if (ctx->reader_status && ctx->reader_status != KS_STATUS_THREAD_STOP_REQUESTED) {
		/* Report a state change to degraded */
		swclt_hstate_changed(&ctx->base, SWCLT_HSTATE_DEGRADED, ctx->reader_status, "Reader failed");
	}

	return NULL;
}

static ks_status_t __start_reader(swclt_wss_ctx_t *ctx)
{
	return ks_thread_create_tag(&ctx->reader_thread, __reader, ctx, ctx->base.pool, "SWClient WSS Reader");
}

// Context

static void __context_deinit(
	swclt_wss_ctx_t *ctx)
{
	ks_log(KS_LOG_INFO, "Shutting down websocket and stopping reader");

	ks_thread_destroy(&ctx->reader_thread);
	ks_handle_destroy(&ctx->rate_send);
	ks_handle_destroy(&ctx->rate_recv);
	ks_thread_destroy(&ctx->reader_thread);
	ks_mutex_destroy(&ctx->write_mutex);
	ks_mutex_destroy(&ctx->read_mutex);
	ks_handle_destroy(&ctx->rate_recv);
	ks_handle_destroy(&ctx->rate_send);
	ks_handle_destroy(&ctx->read_frame);
	kws_destroy(&ctx->wss);
}

static void __context_describe(swclt_wss_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	if (ctx->info.ssl) {
		snprintf(
			buffer,
			buffer_len,
			"SWCLT Websocket connection to: %s:%d/%s (Cipher: %s)",
			ctx->info.address,
			ctx->info.port,
			ctx->info.path,
			ctx->info.cipher
		);
	} else {
		snprintf(
			buffer,
			buffer_len,
			"SWCLT Websocket connection to: %s:%d/%s (No ssl)",
			ctx->info.address,
			ctx->info.port,
			ctx->info.path
		);
	}
}

static ks_status_t __connect_socket(swclt_wss_ctx_t *ctx)
{
	ks_status_t status;
	char buf[256];

	ctx->socket = ks_socket_connect_ex(SOCK_STREAM, IPPROTO_TCP,
			&ctx->addr, ctx->info.connect_timeout_ms);

	if (ctx->socket == KS_SOCK_INVALID) {
		status = KS_STATUS_FAIL;
		goto done;
	}

	snprintf(buf, sizeof(buf), "/%s:%s:swclt", ctx->info.path, ctx->info.address);
	
	if (status = kws_init(&ctx->wss, ctx->socket,
			ctx->info.ssl, buf, KWS_BLOCK | KWS_CLOSE_SOCK, ctx->base.pool))
		goto done;

	/* Load our negotiated cipher while we're here */
	if (ctx->info.ssl) {
		if (status = kws_get_cipher_name(ctx->wss, ctx->info.cipher, sizeof(ctx->info.cipher) - 1))
			goto done;
		ks_log(KS_LOG_INFO, "Websocket negotaited ssl cipher: %s", ctx->info.cipher);
	}

	if (status = __start_reader(ctx))
		goto done;

done:
	if (status) {
		ks_socket_close(&ctx->socket);
		ks_thread_destroy(&ctx->reader_thread);
		kws_destroy(&ctx->wss);
	}

	return status;
}

static ks_status_t __context_init(
	swclt_wss_ctx_t *ctx,
	swclt_wss_incoming_frame_cb_t incoming_frame_cb,
	void *incoming_frame_cb_data,
	const char *address,
	short port,
	const char *path,
	uint32_t timeout_ms,
	const SSL_CTX *ssl)
{
	ks_status_t status;

	ks_log(KS_LOG_INFO, "Web socket initiating connection to: %s on port %u to /%s", address, (unsigned int)port, path);

	ctx->incoming_frame_cb = incoming_frame_cb;
	ctx->incoming_frame_cb_data = incoming_frame_cb_data;
	ctx->info.ssl = (SSL_CTX *)ssl;
	ctx->info.connect_timeout_ms = timeout_ms;
	strncpy(ctx->info.address, address, sizeof(ctx->info.address) - 1);
	strncpy(ctx->info.path, path, sizeof(ctx->info.path) - 1);
	ctx->info.port = port;

	if (status = ks_throughput_create(&ctx->rate_send))
		goto done;
	if (status = ks_throughput_create(&ctx->rate_recv))
		goto done;

	ks_log(KS_LOG_DEBUG, "Resolving address: %s", address);

	if (status = ks_addr_getbyname(address, port, AF_INET, &ctx->addr)) {
		ks_log(KS_LOG_WARNING, "Failed to resolve: %s", address);
		goto done;
	}

	ks_log(KS_LOG_DEBUG, "Successfully resolved address");

	if (status = ks_mutex_create(&ctx->write_mutex, KS_MUTEX_FLAG_DEFAULT, ctx->base.pool))
		goto done;

	if (status = ks_mutex_create(&ctx->read_mutex, KS_MUTEX_FLAG_DEFAULT, ctx->base.pool))
		goto done;

	for (uint32_t tryCount = 0; tryCount < 2; tryCount++) {
		ks_log(KS_LOG_INFO, "Performing connect try: %lu to: %s:%d/%s", tryCount, ctx->info.address, ctx->info.port, ctx->info.path);

		if (status = __connect_socket(ctx)) {
			ks_sleep_ms(1000);
			continue;
		}
		break;
	}

done:
	if (status)
		__context_deinit(ctx);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_wss_connect(
	swclt_wss_t *wss,
	swclt_wss_incoming_frame_cb_t incoming_frame_cb,
	void *incoming_frame_cb_data,
	const char *address,
	short port,
	const char *path,
	uint32_t timeout_ms,
	const SSL_CTX *ssl)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M(
		NULL,
		SWCLT_HTYPE_WSS,
		wss,
		swclt_wss_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		incoming_frame_cb,
		incoming_frame_cb_data,
		address,
		port,
		path,
		timeout_ms,
		ssl);
}

SWCLT_DECLARE(ks_status_t) swclt_wss_get_info(swclt_wss_t wss, swclt_wss_info_t *info)
{
	SWCLT_WSS_SCOPE_BEG(wss, ctx, status)
	memcpy(info, &ctx->info, sizeof(ctx->info));
	SWCLT_WSS_SCOPE_END(wss, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_wss_get_rates(swclt_wss_t wss, ks_throughput_t *send, ks_throughput_t *recv)
{
	SWCLT_WSS_SCOPE_BEG(wss, ctx, status)
	*send = ctx->rate_send;
	*recv = ctx->rate_recv;
	SWCLT_WSS_SCOPE_END(wss, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_wss_write_cmd(swclt_wss_t wss, swclt_cmd_t cmd)
{
	SWCLT_WSS_SCOPE_BEG(wss, ctx, status)
	char *data;
	ks_size_t len, wrote;

	/* Ensure we have valid state */
	if (status = swclt_hstate_check_ctx(&ctx->base, "Write denied, invalid state:"))
		goto ks_handle_scope_end;

	if (status = swclt_cmd_print(cmd, ctx->base.pool, &data)) {
		ks_log(KS_LOG_CRIT, "Invalid command, faild to render payload string: %lu", status);
		goto ks_handle_scope_end;
	}

	len = strlen(data);

	ks_mutex_lock(ctx->write_mutex);
	wrote = kws_write_frame(ctx->wss, WSOC_TEXT, data, len);
	ks_mutex_unlock(ctx->write_mutex);

	ks_log(KS_LOG_DEBUG, "Wrote frame: %s", ks_handle_describe(cmd));

	if (len != wrote)
		status = KS_STATUS_FAIL;
	else
		ks_throughput_report_ex(ctx->rate_send, len, KS_FALSE);

	ks_pool_free(&data);

	SWCLT_WSS_SCOPE_END(wss, ctx, status)
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
