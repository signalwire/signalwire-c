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
#include "signalwire-client-c/internal/handle_monitor.h"

static void __raise_hstate_change(swclt_hmon_ctx_t *ctx, const swclt_hstate_change_t *state_change_info)
{
	if (!ks_handle_valid(ctx->handle_to_monitor)) {
		ks_log(KS_LOG_DEBUG, "Monitored handle event blocked, handle is now invalid");
		return;
	}

	ks_log(KS_LOG_DEBUG, "Raising state change event: %s for monitor handle: %s",
		swclt_hstate_describe_change(state_change_info), ks_handle_describe(ctx->handle_to_monitor));

	ctx->cb(ctx->handle_to_monitor, state_change_info, ctx->cb_data);
}

static void __context_describe(swclt_hmon_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	/* We have to do all this garbage because of the poor decision to nest ks_handle_describe() calls that return a common thread local buffer */
	const char *desc = ks_handle_describe(ctx->handle_to_monitor);
	ks_size_t desc_len = strlen(desc);
	const char *preamble = "SWCLT Handle Monitor for: ";
	ks_size_t preamble_len = strlen(preamble);
	if (desc_len + preamble_len + 1 > buffer_len) {
		desc_len = buffer_len - preamble_len - 1;
	}
	memmove(buffer + preamble_len, desc, desc_len + 1);
	memcpy(buffer, preamble, preamble_len);
	buffer[buffer_len - 1] = '\0';
}

static ks_status_t __context_init(swclt_hmon_ctx_t *ctx, ks_handle_t handle_to_monitor, swclt_hmon_state_change_cb_t cb, void *cb_data)
{
	ctx->handle_to_monitor = handle_to_monitor;
	ctx->cb = cb;
	ctx->cb_data = cb_data;

	/* Associate ourselves a a child of the handle in question */
	return ks_handle_set_parent(ctx->base.handle, handle_to_monitor);
}

static void __context_deinit(swclt_hmon_ctx_t *ctx)
{
}

SWCLT_DECLARE(ks_status_t) __swclt_hmon_register(
	swclt_hmon_t *hmon,
	ks_handle_t handle_to_monitor,
	swclt_hmon_state_change_cb_t cb,
	void *cb_data,
	const char *file,
	int line,
	const char *tag)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M_TAG(
		NULL,
		file,
		line,
		tag,
		SWCLT_HTYPE_HMON,
		hmon,
		swclt_hmon_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		handle_to_monitor,
		cb,
		cb_data);
}

SWCLT_DECLARE(ks_status_t) swclt_hmon_raise_hstate_change(swclt_hmon_t hmon, const swclt_hstate_change_t *state_change_info)
{
	SWCLT_HMON_SCOPE_BEG(hmon, ctx, status)
	__raise_hstate_change(ctx, state_change_info);
	SWCLT_HMON_SCOPE_END(hmon, ctx, status)
}

