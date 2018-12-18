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
#include "signalwire-client-c/transport/internal/frame.h"

static ks_status_t __get_json(swclt_frame_ctx_t *ctx, ks_json_t **json)
{
	if (!ctx->json) { /* Don't redundantly convert */
		if (!(ctx->json = ks_json_pparse(ctx->base.pool, ctx->data))) {
			ks_log(KS_LOG_WARNING, "Failed to parse json");
			return KS_STATUS_INVALID_ARGUMENT;
		}
	}

	if (json)
		*json = ctx->json;

	return KS_STATUS_SUCCESS;
}

static ks_status_t __to_json(swclt_frame_ctx_t *ctx, ks_pool_t *pool, ks_json_t **json)
{
	ks_status_t status;

	if (!ctx->json) {
		if (status = __get_json(ctx, NULL))
			return status;
	}

	if (!(*json = ks_json_pduplicate(pool, ctx->json, KS_TRUE))) {
		return KS_STATUS_NO_MEM;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __to_json_lookup(swclt_frame_ctx_t *ctx, ks_pool_t *pool, ks_json_t **json, uint32_t components, va_list args)
{
	ks_json_t *item;
	ks_status_t status;

	/* Make the json appear */
	if (!ctx->json) {
		if (status = __get_json(ctx, NULL))
			return status;
	}

	/* Do the lookup within our json object and duplicate it for them */
	if (!(item = ks_json_valookup(ctx->json, components, args)))
		return KS_STATUS_NOT_FOUND;

	/* Found it so now dupe it for them */
	if (!(*json = ks_json_pduplicate(pool, item, KS_TRUE)))
		return KS_STATUS_NO_MEM;

	return KS_STATUS_SUCCESS;
}

static ks_status_t __copy_data(swclt_frame_ctx_t *ctx, void *data, ks_size_t len, kws_opcode_t opcode)
{
	/* Invalidate cachhed items */
	ks_json_delete(&ctx->json);
	ks_pool_free(&ctx->cached_description);
	ctx->len = 0;
	ctx->opcode = -1;

	if (!data || opcode == WSOC_INVALID) {
		ks_log(KS_LOG_ERROR, "Unable to copy data because: len = %lu, opcode = %d\n", len, opcode);
		return KS_STATUS_INVALID_ARGUMENT;
	}

	// Note kws returns a size that does not include the final null so account for that here
	if (len > 0) {
		if (ctx->data) {
			if (!(ctx->data = ks_pool_resize(ctx->data, len + 1)))
				return KS_STATUS_NO_MEM;
		} else {
			if (!(ctx->data = ks_pool_alloc(ctx->base.pool, len + 1)))
				return KS_STATUS_NO_MEM;
		}
		memcpy(ctx->data, data, len + 1);
	}


	ctx->opcode = opcode;
	ctx->len = len;
	return KS_STATUS_SUCCESS;
}

static void __context_deinit(swclt_frame_ctx_t *ctx)
{
	ks_json_delete(&ctx->json);
	ks_pool_free(&ctx->data);
}

static ks_status_t __context_init(swclt_frame_ctx_t *ctx)
{
	ctx->opcode = WSOC_INVALID;
	return KS_STATUS_SUCCESS;
}

static void __context_describe(swclt_frame_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	ks_status_t status;

	ks_spinlock_acquire(&ctx->lock);

	if (ctx->opcode == WSOC_TEXT && ctx->data) {
		if (!ctx->cached_description) {
			if (status = __get_json(ctx, NULL)) {
				snprintf(buffer, buffer_len, "SWCLT Frame, failed to render json: %d", status);
				ks_spinlock_release(&ctx->lock);
				return;
			}
			else {
				ctx->cached_description = ks_json_pprint(ctx->base.pool, ctx->json);
			}
		}

		if (ctx->cached_description)
			snprintf(buffer, buffer_len, "SWCLT Frame, opcode: %u, length: %zu, json: %s", ctx->opcode, ctx->len, ctx->cached_description);
#if defined(KS_BUILD_DEBUG)
		else {
			/* Careful writing raw strings to logs heh, just do this for debug for now */
			snprintf(buffer, buffer_len, "SWCLT Frame, opcode: %u, length: %zu, (invalid json) text: %s", ctx->opcode, ctx->len, ctx->data);
		}
#endif
	}
	else if (ctx->opcode != WSOC_INVALID) {
		snprintf(buffer, buffer_len, "SWCLT Frame, opcode: %u, length: %zu", ctx->opcode, ctx->len);
	} else {
		snprintf(buffer, buffer_len, "SWCLT Frame (empy)");
	}

	ks_spinlock_release(&ctx->lock);
}

/**
 * Allocates a frame, wrapped in a handle. A frame is a context used
 * for reading and writing into a kws socket.
 */
SWCLT_DECLARE(ks_status_t) __swclt_frame_alloc(
	swclt_frame_t *frame,
   	const char *file,
   	int line,
   	const char *tag)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_S_TAG(
		NULL,
		file,
		line,
		tag,
		SWCLT_HTYPE_FRAME,
		frame,
		swclt_frame_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init);
}

/**
 * swclt_frame_to_json - Converts the json in the frame, and returns a copy which
 * the owner must release.
 */
SWCLT_DECLARE(ks_status_t) swclt_frame_to_json(swclt_frame_t frame, ks_pool_t *pool, ks_json_t **json)
{
	SWCLT_FRAME_SCOPE_BEG(frame, ctx, status)

	ks_spinlock_acquire(&ctx->lock);
	status = __to_json(ctx, pool, json);
	ks_spinlock_release(&ctx->lock);

	SWCLT_FRAME_SCOPE_END(frame, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_frame_to_json_lookup(swclt_frame_t frame, ks_pool_t *pool, ks_json_t **json, int components, ...)
{
	SWCLT_FRAME_SCOPE_BEG(frame, ctx, status)
	va_list ap;
	va_start(ap, components);

	ks_spinlock_acquire(&ctx->lock);
	status = __to_json_lookup(ctx, pool, json, components, ap);
	ks_spinlock_release(&ctx->lock);

	SWCLT_FRAME_SCOPE_END(frame, ctx, status)
}

/**
 * swclt_frame_get_json - Converts the json in the frame, retains ownership, and
 * returns a copy to the caller.
 */
SWCLT_DECLARE(ks_status_t) swclt_frame_get_json(swclt_frame_t frame, ks_json_t **json)
{
	SWCLT_FRAME_SCOPE_BEG(frame, ctx, status)

	ks_spinlock_acquire(&ctx->lock);
	status = __get_json(ctx, json);
	ks_spinlock_release(&ctx->lock);

	SWCLT_FRAME_SCOPE_END(frame, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_frame_copy_data(swclt_frame_t frame, void *data, ks_size_t len, kws_opcode_t opcode)
{
	SWCLT_FRAME_SCOPE_BEG(frame, ctx, status)
	ks_spinlock_acquire(&ctx->lock);
	status = __copy_data(ctx, data, len, opcode);
	ks_spinlock_release(&ctx->lock);
	SWCLT_FRAME_SCOPE_END(frame, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_frame_get_data(swclt_frame_t frame, void **data, ks_size_t *len, kws_opcode_t *opcode)
{
	SWCLT_FRAME_SCOPE_BEG(frame, ctx, status)
	ks_spinlock_acquire(&ctx->lock);
	*data = ctx->data;
	*len = ctx->len;
	*opcode = ctx->opcode;
	ks_spinlock_release(&ctx->lock);
	SWCLT_FRAME_SCOPE_END(frame, ctx, status)
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
