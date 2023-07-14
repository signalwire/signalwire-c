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

SWCLT_DECLARE(ks_status_t) swclt_sub_destroy(swclt_sub_t **subP)
{
	if (subP && *subP) {
		swclt_sub_t *sub = *subP;
		*subP = NULL;
		ks_pool_free(&sub->protocol);
		ks_pool_free(&sub->channel);
	}
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(char *) swclt_sub_describe(swclt_sub_t *sub)
{
	return ks_psprintf(NULL, "SWCLT Subscription to protocol: %s channel: %s", sub->protocol, sub->channel);
}

SWCLT_DECLARE(ks_status_t) swclt_sub_create(
	swclt_sub_t **subP,
	ks_pool_t *pool,
	const char * const protocol,
	const char * const channel,
	swclt_sub_cb_t cb,
	void *cb_data)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	swclt_sub_t *sub = ks_pool_alloc(pool, sizeof(swclt_sub_t));
	*subP = sub;

	if (!(sub->protocol = ks_pstrdup(NULL, protocol))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	if (!(sub->channel = ks_pstrdup(NULL, channel))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	sub->cb = cb;
	sub->cb_data = cb_data;

done:
	if (status) {
		swclt_sub_destroy(subP);
	}

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_sub_invoke(
	swclt_sub_t *sub,
   	swclt_sess_t *sess,
   	blade_broadcast_rqu_t *broadcast_rqu)
{
	sub->cb(sess, broadcast_rqu, sub->cb_data);
	return KS_STATUS_SUCCESS;
}
