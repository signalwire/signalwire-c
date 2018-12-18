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

/* All subscriptions are handles, opaque numbers that manage ref counts */
typedef ks_handle_t swclt_sub_t;

/* Obfuscate our subscription internals */
typedef struct swclt_sub_ctx swclt_sub_ctx_t;

typedef void (*swclt_sub_cb_t)(
	ks_handle_t sess,
	blade_broadcast_rqu_t *rqu,
	void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_sub_create(
	swclt_sub_t *sub,
	const char * const protocol,
	const char * const channel,
	swclt_sub_cb_t cb,
	void *data);

SWCLT_DECLARE(ks_status_t) swclt_sub_invoke(swclt_sub_t sub, ks_handle_t sess, blade_broadcast_rqu_t *broadcast_rqu);

#define swclt_sub_get(sub, contextP)		__ks_handle_get(SWCLT_HTYPE_SUB, sub, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_sub_put(contextP)			__ks_handle_put(SWCLT_HTYPE_SUB, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)


KS_END_EXTERN_C
