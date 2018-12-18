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
#include "signalwire-client-c/internal/subscription.h"

static void __context_deinit(swclt_sub_ctx_t *ctx)
{
	ks_pool_free(&ctx->protocol);
	ks_pool_free(&ctx->channel);
}

static void __context_describe(swclt_sub_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	snprintf(buffer, buffer_len, "SWCLT Subscription to protocol: %s channel: %s", ctx->protocol, ctx->channel);
}

static ks_status_t __context_init(
	swclt_sub_ctx_t *ctx,
	const char * const protocol,
	const char * const channel,
	swclt_sub_cb_t cb,
	void *cb_data)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	if (!(ctx->protocol = ks_pstrdup(NULL, protocol))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	if (!(ctx->channel = ks_pstrdup(NULL, channel))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	ctx->cb = cb;
	ctx->cb_data = cb_data;

done:
	if (status)
		__context_deinit(ctx);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sub_create(
	swclt_sub_t *sub,
	const char * const protocol,
	const char * const channel,
	swclt_sub_cb_t callback,
	void *cb_data)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M(
		NULL,
		SWCLT_HTYPE_SUB,
		sub,
		swclt_sub_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		protocol,
		channel,
		callback,
		cb_data)
}

SWCLT_DECLARE(ks_status_t) swclt_sub_invoke(
	swclt_sub_t sub,
   	swclt_sess_t sess,
   	blade_broadcast_rqu_t *broadcast_rqu)
{
	SWCLT_SUB_SCOPE_BEG(sub, ctx, status)
	ctx->cb(sess, broadcast_rqu, ctx->cb_data);
	SWCLT_SUB_SCOPE_END(sub, ctx, status)
}
