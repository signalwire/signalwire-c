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

/* The internal context wrapped by the frame handle */
struct swclt_frame_ctx {
	swclt_handle_base_t base;

	/* Raw data read from the socket */
	ks_size_t len;
	uint8_t *data;

	/* The frame may be manipulated by multiple threads from time to time, so 
	 * a lightweight lock is a good idea */
	ks_spinlock_t lock;

	/* Cached description */
	const char *cached_description;

	/* Parsed data cached */
	ks_json_t *json;

	/* The operation code for the socket */
	kws_opcode_t opcode;
};

KS_END_EXTERN_C

/* Define helper macros to eliminate boiler code in handle wrapped apis */
#define SWCLT_FRAME_SCOPE_BEG(frame, ctx, status) \
	KS_HANDLE_SCOPE_BEG(SWCLT_HTYPE_FRAME, frame, swclt_frame_ctx_t, ctx, status);

#define SWCLT_FRAME_SCOPE_END(frame, ctx, status) \
	KS_HANDLE_SCOPE_END(SWCLT_HTYPE_FRAME, frame, swclt_frame_ctx_t, ctx, status);

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

