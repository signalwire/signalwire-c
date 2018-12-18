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

struct swclt_hmon_ctx {
	swclt_handle_base_t base;

	/* This is the handle the caller wants us to monitor */
	ks_handle_t handle_to_monitor;

	/* The callers callback and callback data */
	swclt_hmon_state_change_cb_t cb;
	void *cb_data;
};

KS_END_EXTERN_C

/* Define helper macros to eliminate boiler code in handle wrapped apis */
#define SWCLT_HMON_SCOPE_BEG(hmon, ctx, status) \
	KS_HANDLE_SCOPE_BEG(SWCLT_HTYPE_HMON, hmon, swclt_hmon_ctx_t, ctx, status);

#define SWCLT_HMON_SCOPE_END(hmon, ctx, status) \
	KS_HANDLE_SCOPE_END(SWCLT_HTYPE_HMON, hmon, swclt_hmon_ctx_t, ctx, status);

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
