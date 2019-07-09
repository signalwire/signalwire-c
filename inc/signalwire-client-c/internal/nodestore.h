/*
 * Copyright (c) 2018-2019 SignalWire, Inc
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

struct swclt_store_ctx {
	swclt_handle_base_t base;

	ks_pool_t *pool;

	// callbacks keyed by netcast command, pointing to value of callback to call
	ks_hash_t *callbacks;
	
	/* Hash of nodes keyed by their nodeid. They point to a Node
	 * class which contains their certified status. */
	ks_hash_t *routes;

	/* Hash keyed by identity mapped to nodeid */
	ks_hash_t *identities;

	/* Last index position we selected for random protocol gathering */
	uint32_t last_random_protocol_idx;

	/* Hash of protocols, keyed by the protocol name, each protocol
	 * contains channels. */
	ks_hash_t *protocols;

	/* Hash of Subscription objects, keyed by their protocol name. */
	ks_hash_t *subscriptions;

	/* Hash of authorities, keyed by their node uuid. */
	ks_hash_t *authorities;

	/* Hash of protocols available to uncertified clients only, keyed by protocol name */
	ks_hash_t *protocols_uncertified;
};

/* Define helper macros to eliminate boiler code in handle wrapped apis */
#define SWCLT_STORE_SCOPE_BEG(store, ctx, status) \
	KS_HANDLE_SCOPE_BEG(SWCLT_HTYPE_STORE, store, swclt_store_ctx_t, ctx, status);

#define SWCLT_STORE_SCOPE_END(store, ctx, status) \
	KS_HANDLE_SCOPE_END(SWCLT_HTYPE_STORE, store, swclt_store_ctx_t, ctx, status);

KS_END_EXTERN_C
