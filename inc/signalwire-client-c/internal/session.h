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

#include "signalwire-client-c/internal/connection.h"

KS_BEGIN_EXTERN_C

/* Create a structure containing some handy peices of info */
typedef struct swclt_sess_info_s {
	/* The info structure from our connection (it itself then contains
	 * an info structure for the transport) */
	swclt_conn_info_t conn;

	/* Copied from the connect result in the connection info, used
	 * as a shortcut for convenience */
	ks_uuid_t sessionid;
	const char *nodeid;
	const char *master_nodeid;
} swclt_sess_info_t;

/* Ths client session context represents a sessions state */
struct swclt_sess_ctx {
	swclt_handle_base_t base;

	/* The pool that manages all the allocations */
	ks_pool_t *pool;

	/* The node store, an api to keep the cache in sync with blade, contains
	 * a lotta info about stuff. */
	swclt_store_t store;

	/* Our connection */
	swclt_conn_t conn;

	/* The extracted identity info */
	swclt_ident_t ident;

	/* Our info structure */
	swclt_sess_info_t info;

	SSL_CTX *ssl;

	/* Our config handed to us by the client */
	swclt_config_t *config;

	/* Optional callback for authentication failure */
	swclt_sess_auth_failed_cb_t auth_failed_cb;

	/* We keep track of subscriptions in a hash here, each call to subscribe
	 * from the client will add an entry in this hash. The hash points to the
	 * subscription handle which will contain the users callback */
	ks_hash_t *subscriptions;

	/* Registry for Protocol RPC methods */
	ks_hash_t *methods;

	/* Setups completed for gandalf provider event channels */
	ks_hash_t *setups;

	/* Registry for metric rank updates for local protocols */
	ks_hash_t *metrics;
};

KS_END_EXTERN_C

/* Define helper macros to eliminate boiler code in handle wrapped apis */
#define SWCLT_SESS_SCOPE_BEG(sess, ctx, status) \
	KS_HANDLE_SCOPE_BEG(SWCLT_HTYPE_SESS, sess, swclt_sess_ctx_t, ctx, status);

/* Semicolon at start of macro ensures it does not create a compound error
 * if a goto label appears just before this macro */
#define SWCLT_SESS_SCOPE_END(sess, ctx, status) \
	;KS_HANDLE_SCOPE_END(SWCLT_HTYPE_SESS, sess, swclt_sess_ctx_t, ctx, status);

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
