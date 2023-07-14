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

#include "signalwire-client-c/client.h"

// Write apis

static ks_status_t __write_raw(swclt_wss_t *ctx, kws_opcode_t opcode, const void *data, ks_size_t len)
{
	ks_size_t wrote;

	ks_log(KS_LOG_DEBUG, "Writing frame of size: %lu opcode: %lu\n", len, opcode);

	ks_mutex_lock(ctx->wss_mutex);
	wrote = kws_write_frame(ctx->wss, opcode, data, len);
	if (wrote > 0) {
		ctx->stats.write_frames++;
	}
	ks_mutex_unlock(ctx->wss_mutex);

	if (wrote != len) {
		ks_log(KS_LOG_WARNING, "Failed to write frame\n");
		return KS_STATUS_FAIL;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __write_ping(swclt_wss_t *ctx)
{
	static uint64_t data = 0;
	return __write_raw(ctx, WSOC_PING, &data, sizeof(data));
}

static ks_status_t __write_pong(swclt_wss_t *ctx, swclt_frame_t *frame)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_ssize_t wrote;

	ks_mutex_lock(ctx->wss_mutex);
	if ((wrote = kws_write_frame(ctx->wss, WSOC_PONG, frame->data, frame->len)) != frame->len) {
		ks_log(KS_LOG_WARNING, "Failed to write pong");
		status = KS_STATUS_FAIL;
	}
	if (wrote > 0) {
		ctx->stats.write_frames++;
	}
	ks_mutex_unlock(ctx->wss_mutex);
	return status;
}

// Read apis

static ks_status_t __poll_read(swclt_wss_t *ctx, uint32_t *poll_flagsP)
{
	*poll_flagsP = ks_wait_sock(ctx->socket, 1000, KS_POLL_READ);
	if (*poll_flagsP & KS_POLL_ERROR) {
		return KS_STATUS_FAIL;
	}

	return KS_STATUS_SUCCESS;
}

// Read thread
static ks_status_t __read_frame(swclt_wss_t *ctx, swclt_frame_t **frameP, kws_opcode_t *opcodeP)
{
	swclt_frame_t *frame = NULL;
	kws_opcode_t opcode = WSOC_INVALID;
	uint8_t *data;
	ks_ssize_t len;
	ks_status_t status;

	/* Allocate a new frame off the global pool if needed */
	if (!*frameP) {
		if (status = swclt_frame_alloc(&frame, NULL)) {
			return status;
		}
	} else {
		frame = *frameP;

		/* Mark it null for now if we error its getting freed */
		*frameP = NULL;
	}

	ks_mutex_lock(ctx->wss_mutex);

	/* kws will actually retain ownership of its buffer so, we'll want to make a copy
	 * into our frame */
	len = kws_read_frame(ctx->wss, &opcode, &data);

	if (len < 0) {
		ks_log(KS_LOG_WARNING, "Read frame failed");
		status = KS_STATUS_NO_MEM;
		goto done;
	}
	ctx->stats.read_frames++;

	ks_log(KS_LOG_DEBUG, "Copying frame of length: %lu of opcode: %lu", (size_t)len, opcode);

	/* Stash it in the frame */
	if (status = swclt_frame_copy_data(frame, NULL, data, (size_t)len, opcode)) {
		goto done;
	}

done:
	ks_mutex_unlock(ctx->wss_mutex);

	if (status) {
		ks_pool_free(&frame);
	} else {
		*frameP = frame;
		*opcodeP = opcode;
	}

	return status;
}
static ks_status_t __reader_loop(swclt_wss_t *ctx)
{
	ks_status_t status;

	ks_log(KS_LOG_DEBUG, "Websocket reader starting");

	/* Loop until the caller requests a stop */
	while (!ks_thread_stop_requested(ctx->reader_thread)) {
		uint32_t poll_flags;
		kws_opcode_t frame_opcode;

		if (ctx->failed) {
			return KS_STATUS_FAIL;
		}

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

			if (ctx->read_frame->len > 0) {
				ks_log(KS_LOG_DEBUG, "Read frame: opcode %u, length %zu, data: %s", ctx->read_frame->opcode, ctx->read_frame->len, ctx->read_frame->data);
			} else {
				ks_log(KS_LOG_DEBUG, "Read frame: opcode %u, length %zu, data: (null)", ctx->read_frame->opcode, ctx->read_frame->len);
			}

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
				if (status = ctx->incoming_frame_cb(ctx, &ctx->read_frame, ctx->incoming_frame_cb_data)) {
					ks_log(KS_LOG_WARNING, "Callback from incoming frame returned: %d, exiting", status);
					ks_pool_free(&ctx->read_frame);
					return status;
				}
				ks_pool_free(&ctx->read_frame);
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
	swclt_wss_t *ctx = (swclt_wss_t *)data;
	ctx->reader_status = __reader_loop(ctx);

	if (ctx->reader_status && ctx->reader_status != KS_STATUS_THREAD_STOP_REQUESTED) {
		/* Report a failed state */
		ctx->failed = 1;
		if (ctx->failed_cb) {
			ctx->failed_cb(ctx, ctx->failed_cb_data);
		}
	}

	return NULL;
}

static ks_status_t __start_reader(swclt_wss_t *ctx)
{
	return ks_thread_create_tag(&ctx->reader_thread, __reader, ctx, ctx->pool, "swclt-wss-reader");
}


SWCLT_DECLARE(char *) swclt_wss_describe(swclt_wss_t *ctx)
{
	if (ctx->info.ssl) {
		return ks_psprintf(ctx->pool,
			"SWCLT Websocket connection to: %s:%d/%s (Cipher: %s)",
			ctx->info.address,
			ctx->info.port,
			ctx->info.path,
			ctx->info.cipher
		);
	}
	return ks_psprintf(
		ctx->pool,
		"SWCLT Websocket connection to: %s:%d/%s (No ssl)",
		ctx->info.address,
		ctx->info.port,
		ctx->info.path
	);
}

static ks_status_t __connect_socket(swclt_wss_t *ctx)
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
			ctx->info.ssl, buf, KWS_BLOCK | KWS_CLOSE_SOCK, ctx->pool))
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
		if (ctx->reader_thread) {
			ks_thread_request_stop(ctx->reader_thread);
		}
		ks_socket_close(&ctx->socket);
		if (ctx->reader_thread) {
			ks_thread_join(ctx->reader_thread);
			ks_thread_destroy(&ctx->reader_thread);
		}
		kws_destroy(&ctx->wss);
	}

	return status;
}

SWCLT_DECLARE(void) swclt_wss_destroy(swclt_wss_t **wss)
{
	if (wss && *wss) {
		ks_pool_t *pool = (*wss)->pool;
		ks_log(KS_LOG_INFO, "Shutting down websocket");
		if ((*wss)->reader_thread) {
			ks_thread_request_stop((*wss)->reader_thread);
		}
		ks_socket_close(&(*wss)->socket);
		if ((*wss)->reader_thread) {
			ks_thread_join((*wss)->reader_thread);
			ks_thread_destroy(&(*wss)->reader_thread);
		}
		if ((*wss)->wss_mutex) {
			ks_mutex_destroy(&(*wss)->wss_mutex);
		}
		ks_pool_free(&(*wss)->read_frame);
		kws_destroy(&(*wss)->wss);
		ks_pool_free(wss);
		ks_pool_close(&pool);
	}
}

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
	const SSL_CTX *ssl)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_pool_t *pool = NULL;
	ks_pool_open(&pool);
	swclt_wss_t *new_wss = ks_pool_alloc(pool, sizeof(swclt_wss_t));
	new_wss->pool = pool;

	ks_log(KS_LOG_INFO, "Web socket initiating connection to: %s on port %u to /%s", address, (unsigned int)port, path);

	new_wss->incoming_frame_cb = incoming_frame_cb;
	new_wss->incoming_frame_cb_data = incoming_frame_cb_data;
	new_wss->failed_cb = failed_cb;
	new_wss->failed_cb_data = failed_cb_data;
	new_wss->info.ssl = (SSL_CTX *)ssl;
	new_wss->info.connect_timeout_ms = timeout_ms;
	strncpy(new_wss->info.address, address, sizeof(new_wss->info.address) - 1);
	strncpy(new_wss->info.path, path, sizeof(new_wss->info.path) - 1);
	new_wss->info.port = port;

	ks_log(KS_LOG_DEBUG, "Resolving address: %s", address);

	if (status = ks_addr_getbyname(address, port, AF_INET, &new_wss->addr)) {
		ks_log(KS_LOG_WARNING, "Failed to resolve: %s", address);
		goto done;
	}

	ks_log(KS_LOG_DEBUG, "Successfully resolved address");

	if (status = ks_mutex_create(&new_wss->wss_mutex, KS_MUTEX_FLAG_DEFAULT, new_wss->pool))
		goto done;

	for (uint32_t tryCount = 0; tryCount < 2; tryCount++) {
		ks_log(KS_LOG_INFO, "Performing connect try: %lu to: %s:%d/%s", tryCount, new_wss->info.address, new_wss->info.port, new_wss->info.path);

		if (status = __connect_socket(new_wss)) {
			ks_sleep_ms(1000);
			continue;
		}
		break;
	}

done:
	if (status != KS_STATUS_SUCCESS) {
		swclt_wss_destroy(&new_wss);
	}
	*wss = new_wss;
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_wss_get_info(swclt_wss_t *wss, swclt_wss_info_t *info)
{
	memcpy(info, &wss->info, sizeof(wss->info));
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(void) swclt_wss_get_stats(swclt_wss_t *ctx, swclt_wss_stats_t *stats)
{
	ks_mutex_lock(ctx->wss_mutex);
	memcpy(stats, &ctx->stats, sizeof(ctx->stats));
	ks_mutex_unlock(ctx->wss_mutex);
}

SWCLT_DECLARE(ks_status_t) swclt_wss_write(swclt_wss_t *wss, char *data)
{
	ks_size_t len;
	ks_ssize_t wrote;
	ks_status_t status;

	/* Ensure we have valid state */
	if (wss->failed) {
		return KS_STATUS_FAIL;
	}

	len = strlen(data);

	ks_mutex_lock(wss->wss_mutex);
	wrote = kws_write_frame(wss->wss, WSOC_TEXT, data, len);
	if (wrote > 0) {
		wss->stats.write_frames++;
	}
	ks_mutex_unlock(wss->wss_mutex);

	if (wrote < 0 || len != (ks_size_t)wrote) {
		ks_log(KS_LOG_WARNING, "Short write to websocket.  wrote = %d, len = %u", wrote, len);
		status = KS_STATUS_FAIL;
		wss->failed = 1;
		if (wss->failed_cb) {
			wss->failed_cb(wss, wss->failed_cb_data);
		}
	} else {
		ks_log(KS_LOG_DEBUG, "Wrote frame: %s", data);
		status = KS_STATUS_SUCCESS;
	}

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
