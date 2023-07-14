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

struct swclt_store {
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

	swclt_sess_t *sess;
};

typedef struct swclt_store swclt_store_t;

typedef void (*swclt_store_cb_protocol_add_t)(swclt_sess_t *sess,
											  const char *protocol);

typedef void (*swclt_store_cb_protocol_remove_t)(swclt_sess_t *sess,
												 const char *protocol);

typedef void (*swclt_store_cb_protocol_provider_add_t)(swclt_sess_t *sess,
	const blade_netcast_rqu_t *rqu,
	const blade_netcast_protocol_provider_add_param_t *params);

typedef void (*swclt_store_cb_protocol_provider_remove_t)(
	swclt_sess_t *sess,
	const blade_netcast_rqu_t* rqu,
	const blade_netcast_protocol_provider_remove_param_t *params);

typedef void (*swclt_store_cb_protocol_provider_rank_update_t)(swclt_sess_t *sess,
	const blade_netcast_rqu_t *rqu,
	const blade_netcast_protocol_provider_rank_update_param_t *params);

typedef void (*swclt_store_cb_protocol_provider_data_update_t)(swclt_sess_t *sess,
	const blade_netcast_rqu_t *rqu,
	const blade_netcast_protocol_provider_data_update_param_t *params);

typedef void (*swclt_store_cb_route_add_t)(swclt_sess_t *sess,
	const blade_netcast_rqu_t *rqu,
	const blade_netcast_route_add_param_t *params);

typedef void (*swclt_store_cb_route_remove_t)(
	swclt_sess_t *sess,
	const blade_netcast_rqu_t* rqu,
	const blade_netcast_route_remove_param_t *params);

typedef void (*swclt_store_cb_authority_add_t)(swclt_sess_t *sess,
	const blade_netcast_rqu_t *rqu,
	const blade_netcast_authority_add_param_t *params);

typedef void (*swclt_store_cb_authority_remove_t)(
	swclt_sess_t *sess,
	const blade_netcast_rqu_t* rqu,
	const blade_netcast_authority_remove_param_t *params);

typedef void (*swclt_store_cb_subscription_add_t)(swclt_sess_t *sess,
	const blade_netcast_rqu_t *rqu,
	const blade_netcast_subscription_add_param_t *params);

typedef void (*swclt_store_cb_subscription_remove_t)(
	swclt_sess_t *sess,
	const blade_netcast_rqu_t* rqu,
	const blade_netcast_subscription_remove_param_t *params);

typedef void (*swclt_store_cb_identity_add_t)(swclt_sess_t *sess,
	const blade_netcast_rqu_t *rqu,
	const blade_netcast_identity_add_param_t *params);

typedef void (*swclt_store_cb_identity_remove_t)(
	swclt_sess_t *sess,
	const blade_netcast_rqu_t* rqu,
	const blade_netcast_identity_remove_param_t *params);

SWCLT_DECLARE(ks_status_t) swclt_store_create(swclt_store_t **store);
SWCLT_DECLARE(ks_status_t) swclt_store_destroy(swclt_store_t **store);
SWCLT_DECLARE(char *) swclt_store_describe(swclt_store_t *store);
SWCLT_DECLARE(ks_status_t) swclt_store_reset(swclt_store_t *store);
SWCLT_DECLARE(ks_status_t) swclt_store_populate(swclt_store_t *store, const blade_connect_rpl_t *connect_rpl);
SWCLT_DECLARE(ks_status_t) swclt_store_update(swclt_store_t *store, const blade_netcast_rqu_t *netcast_rqu);
SWCLT_DECLARE(ks_status_t) swclt_store_get_node_identities(swclt_store_t *store, const char *nodeid, ks_pool_t *pool, ks_hash_t **identities);
SWCLT_DECLARE(ks_status_t) swclt_store_get_protocols(swclt_store_t *store, ks_json_t **protocols);
SWCLT_DECLARE(ks_status_t) swclt_store_check_protocol(swclt_store_t *store, const char *name);

SWCLT_DECLARE(ks_status_t) swclt_store_select_random_protocol_provider(
	swclt_store_t *store,
   	const char *name,
	ks_pool_t *pool,
	char **providerid);

SWCLT_DECLARE(ks_status_t) swclt_store_get_protocol_providers(
	swclt_store_t *store,
	const char *name,
	ks_json_t **providers);

SWCLT_DECLARE(ks_status_t) swclt_store_cb_route_add(swclt_store_t *store, swclt_store_cb_route_add_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_route_remove(swclt_store_t *store, swclt_store_cb_route_remove_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_identity_add(swclt_store_t *store, swclt_store_cb_identity_add_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_identity_remove(swclt_store_t *store, swclt_store_cb_identity_remove_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_add(swclt_store_t *store, swclt_store_cb_protocol_add_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_remove(swclt_store_t *store, swclt_store_cb_protocol_remove_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_add(swclt_store_t *store, swclt_store_cb_protocol_provider_add_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_remove(swclt_store_t *store, swclt_store_cb_protocol_provider_remove_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_rank_update(swclt_store_t *store, swclt_store_cb_protocol_provider_rank_update_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_data_update(swclt_store_t *store, swclt_store_cb_protocol_provider_data_update_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_authority_add(swclt_store_t *store, swclt_store_cb_authority_add_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_authority_remove(swclt_store_t *store, swclt_store_cb_authority_remove_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_subscription_add(swclt_store_t *store, swclt_store_cb_subscription_add_t cb);
SWCLT_DECLARE(ks_status_t) swclt_store_cb_subscription_remove(swclt_store_t *store, swclt_store_cb_subscription_remove_t cb);

KS_END_EXTERN_C
