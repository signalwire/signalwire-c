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

KS_BEGIN_EXTERN_C

typedef struct swclt_sess swclt_sess_t;

typedef struct swclt_sub swclt_sub_t;

typedef void (*swclt_sub_cb_t)(
	swclt_sess_t *sess,
	blade_broadcast_rqu_t *rqu,
	void *cb_data);

/* A subscription represents a logical registration of
 * a listener on a channel */
struct swclt_sub {
	const char *protocol;
	const char *channel;

	swclt_sub_cb_t cb;

	void *cb_data;
};


SWCLT_DECLARE(ks_status_t) swclt_sub_create(
	swclt_sub_t **sub,
	ks_pool_t *pool,
	const char * const protocol,
	const char * const channel,
	swclt_sub_cb_t cb,
	void *data);

SWCLT_DECLARE(ks_status_t) swclt_sub_invoke(swclt_sub_t *sub, swclt_sess_t *sess, blade_broadcast_rqu_t *broadcast_rqu);
SWCLT_DECLARE(char *) swclt_sub_describe(swclt_sub_t *sub);
SWCLT_DECLARE(ks_status_t) swclt_sub_destroy(swclt_sub_t **sub);

KS_END_EXTERN_C
