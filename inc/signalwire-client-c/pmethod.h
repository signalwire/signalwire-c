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

#pragma once

/**
 * A pmethod is a registered execution handler bound to a protocol and channel.
 * Unlike some other aspects of protocols, an execution handler need not register
 * the protocol as a provider, nor does the protocol need to exist. Two nodes
 * may execute between eachother without either of these constructs in place.
 */

KS_BEGIN_EXTERN_C

typedef ks_status_t (*swclt_pmethod_cb_t)(
	swclt_sess_t *sess,
	swclt_cmd_t *cmd,
	const blade_execute_rqu_t *rqu, void *cb_data);

/**
 * swclt_pmethod_cb_t - This context structure is held in a session and is keyed by the protocol
 * and channel.
 */
typedef struct swclt_pmethod_ctx_s {
	swclt_pmethod_cb_t cb;
	void *cb_data;
} swclt_pmethod_ctx_t;

KS_END_EXTERN_C
