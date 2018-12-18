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

/* Obfuscate the frame context internals */
typedef struct swclt_frame_ctx swclt_frame_ctx_t;

typedef ks_handle_t swclt_frame_t;

KS_BEGIN_EXTERN_C

SWCLT_DECLARE(ks_status_t) __swclt_frame_alloc(swclt_frame_t *frame, const char *file, int line, const char *tag);
#define swclt_frame_alloc(frame)	__swclt_frame_alloc(frame, __FILE__, __LINE__, __PRETTY_FUNCTION__)

SWCLT_DECLARE(ks_status_t) swclt_frame_get_json(swclt_frame_t frame, ks_json_t **json);

SWCLT_DECLARE(ks_status_t) swclt_frame_to_json(swclt_frame_t frame, ks_pool_t *pool, ks_json_t **json);
SWCLT_DECLARE(ks_status_t) swclt_frame_to_json_lookup(swclt_frame_t frame, ks_pool_t *pool, ks_json_t **json, int components, ...);

SWCLT_DECLARE(ks_status_t) swclt_frame_copy_data(swclt_frame_t frame,  void *data, ks_size_t len, kws_opcode_t opcode);
SWCLT_DECLARE(ks_status_t) swclt_frame_get_data(swclt_frame_t frame, void **data, ks_size_t *len, kws_opcode_t *opcode);

#define swclt_frame_get(frame, contextP)		__ks_handle_get(SWCLT_HTYPE_FRAME, frame, (ks_handle_base_t **)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_frame_put(contextP)		ks_handle_put(SWCLT_HTYPE_FRAME, contextP)

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
