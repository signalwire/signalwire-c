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

#include "signalwire-client-c/client.h"
#include "signalwire-client-c/internal/nodestore.h"

// @todo callbacks should be stored in internal lists of callbacks to support multiple consumers
// @todo if userdata is added requiring a structure of data, then maybe wrap within a handle as well?

static swclt_sess_t __get_sess_from_store_ctx(swclt_store_ctx_t *ctx)
{
	ks_handle_t parent;
	ks_handle_parent(ctx->base.handle, &parent);
	return parent;
}

static ks_status_t __add_cb_route_add(swclt_store_ctx_t *ctx, swclt_store_cb_route_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding route add handler for method: %s", BLADE_NETCAST_CMD_ROUTE_ADD);
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_ROUTE_ADD, (void *)cb);
}

static void __invoke_cb_route_add(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_route_add_param_t *params)
{
	swclt_store_cb_route_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up route add handler for method: %s", BLADE_NETCAST_CMD_ROUTE_ADD);

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_route_add_t)ks_hash_search(ctx->callbacks,
													(const void *)BLADE_NETCAST_CMD_ROUTE_ADD,
													KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store add");
		cb(__get_sess_from_store_ctx(ctx), rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for route add method: %s", BLADE_NETCAST_CMD_ROUTE_ADD);
	}
}

static ks_status_t __add_cb_route_remove(swclt_store_ctx_t *ctx, swclt_store_cb_route_remove_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_ROUTE_REMOVE, (void *)cb);
}

static void __invoke_cb_route_remove(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_route_remove_param_t *params)
{
	swclt_store_cb_route_remove_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_route_remove_t)ks_hash_search(ctx->callbacks,
													   (const void *)BLADE_NETCAST_CMD_ROUTE_REMOVE,
													   KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static ks_status_t __add_cb_identity_add(swclt_store_ctx_t *ctx, swclt_store_cb_identity_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding identity add handler for method: %s", BLADE_NETCAST_CMD_IDENTITY_ADD);
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_IDENTITY_ADD, (void *)cb);
}

static void __invoke_cb_identity_add(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_identity_add_param_t *params)
{
	swclt_store_cb_identity_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up identity add handler for method: %s", BLADE_NETCAST_CMD_IDENTITY_ADD);

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_identity_add_t)ks_hash_search(ctx->callbacks,
													   (const void *)BLADE_NETCAST_CMD_IDENTITY_ADD,
													   KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store identity add");
		cb(__get_sess_from_store_ctx(ctx), rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for identity add method: %s", BLADE_NETCAST_CMD_IDENTITY_ADD);
	}
}

static ks_status_t __add_cb_identity_remove(swclt_store_ctx_t *ctx, swclt_store_cb_identity_remove_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_IDENTITY_REMOVE, (void *)cb);
}

static void __invoke_cb_identity_remove(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_identity_remove_param_t *params)
{
	swclt_store_cb_identity_remove_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_identity_remove_t)ks_hash_search(ctx->callbacks,
														  (const void *)BLADE_NETCAST_CMD_IDENTITY_REMOVE,
														  KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static ks_status_t __add_cb_protocol_add(swclt_store_ctx_t *ctx, swclt_store_cb_protocol_add_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_ADD, (void *)cb);
}

static void __invoke_cb_protocol_add(swclt_store_ctx_t *ctx,
									 const char *protocol)
{
	swclt_store_cb_protocol_add_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_protocol_add_t)ks_hash_search(ctx->callbacks,
													   (const void *)BLADE_NETCAST_CMD_PROTOCOL_ADD,
													   KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), protocol);
}

static ks_status_t __add_cb_protocol_remove(swclt_store_ctx_t *ctx, swclt_store_cb_protocol_remove_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_REMOVE, (void *)cb);
}

static void __invoke_cb_protocol_remove(swclt_store_ctx_t *ctx,
										const char *protocol)
{
	swclt_store_cb_protocol_remove_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_protocol_remove_t)ks_hash_search(ctx->callbacks,
														  (const void *)BLADE_NETCAST_CMD_PROTOCOL_REMOVE,
														  KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), protocol);
}

static ks_status_t __add_cb_protocol_provider_add(swclt_store_ctx_t *ctx, swclt_store_cb_protocol_provider_add_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD, (void *)cb);
}

static void __invoke_cb_protocol_provider_add(swclt_store_ctx_t *ctx,
											  const blade_netcast_rqu_t *rqu,
											  const blade_netcast_protocol_provider_add_param_t *params)
{
	swclt_store_cb_protocol_provider_add_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_protocol_provider_add_t)ks_hash_search(ctx->callbacks,
																(const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD,
																KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static ks_status_t __add_cb_protocol_provider_remove(swclt_store_ctx_t *ctx, swclt_store_cb_protocol_provider_remove_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE, (void *)cb);
}

static void __invoke_cb_protocol_provider_remove(swclt_store_ctx_t *ctx,
												 const blade_netcast_rqu_t *rqu,
												 const blade_netcast_protocol_provider_remove_param_t *params)
{
	swclt_store_cb_protocol_provider_remove_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_protocol_provider_remove_t)ks_hash_search(ctx->callbacks,
																   (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE,
																   KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static ks_status_t __add_cb_protocol_provider_rank_update(swclt_store_ctx_t *ctx, swclt_store_cb_protocol_provider_rank_update_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE, (void *)cb);
}

static void __invoke_cb_protocol_provider_rank_update(swclt_store_ctx_t *ctx,
													  const blade_netcast_rqu_t *rqu,
													  const blade_netcast_protocol_provider_rank_update_param_t *params)
{
	swclt_store_cb_protocol_provider_rank_update_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_protocol_provider_rank_update_t)ks_hash_search(ctx->callbacks,
																		(const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE,
																		KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static ks_status_t __add_cb_protocol_provider_data_update(swclt_store_ctx_t *ctx, swclt_store_cb_protocol_provider_data_update_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE, (void *)cb);
}

static void __invoke_cb_protocol_provider_data_update(swclt_store_ctx_t *ctx,
													  const blade_netcast_rqu_t *rqu,
													  const blade_netcast_protocol_provider_data_update_param_t *params)
{
	swclt_store_cb_protocol_provider_data_update_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_protocol_provider_data_update_t)ks_hash_search(ctx->callbacks,
																		(const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE,
																		KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static ks_status_t __add_cb_authority_add(swclt_store_ctx_t *ctx, swclt_store_cb_authority_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding authority add handler for method: %s", BLADE_NETCAST_CMD_AUTHORITY_ADD);
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_AUTHORITY_ADD, (void *)cb);
}

static void __invoke_cb_authority_add(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_authority_add_param_t *params)
{
	swclt_store_cb_authority_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up authority add handler for method: %s", BLADE_NETCAST_CMD_AUTHORITY_ADD);

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_authority_add_t)ks_hash_search(ctx->callbacks,
														(const void *)BLADE_NETCAST_CMD_AUTHORITY_ADD,
														KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store add");
		cb(__get_sess_from_store_ctx(ctx), rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for authority add method: %s", BLADE_NETCAST_CMD_AUTHORITY_ADD);
	}
}

static ks_status_t __add_cb_authority_remove(swclt_store_ctx_t *ctx, swclt_store_cb_authority_remove_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_AUTHORITY_REMOVE, (void *)cb);
}

static void __invoke_cb_authority_remove(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_authority_remove_param_t *params)
{
	swclt_store_cb_authority_remove_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_authority_remove_t)ks_hash_search(ctx->callbacks,
														   (const void *)BLADE_NETCAST_CMD_AUTHORITY_REMOVE,
														   KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static ks_status_t __add_cb_subscription_add(swclt_store_ctx_t *ctx, swclt_store_cb_subscription_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding authority add handler for method: %s", BLADE_NETCAST_CMD_SUBSCRIPTION_ADD);
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_ADD, (void *)cb);
}

static void __invoke_cb_subscription_add(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_subscription_add_param_t *params)
{
	swclt_store_cb_subscription_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up subscription add handler for method: %s", BLADE_NETCAST_CMD_SUBSCRIPTION_ADD);

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_subscription_add_t)ks_hash_search(ctx->callbacks,
														(const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_ADD,
														KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store add");
		cb(__get_sess_from_store_ctx(ctx), rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for subscription add method: %s", BLADE_NETCAST_CMD_SUBSCRIPTION_ADD);
	}
}

static ks_status_t __add_cb_subscription_remove(swclt_store_ctx_t *ctx, swclt_store_cb_subscription_remove_t cb)
{
	return ks_hash_insert(ctx->callbacks, (const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE, (void *)cb);
}

static void __invoke_cb_subscription_remove(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *rqu, const blade_netcast_subscription_remove_param_t *params)
{
	swclt_store_cb_subscription_remove_t cb;

	ks_hash_read_lock(ctx->callbacks);
	cb = (swclt_store_cb_subscription_remove_t)ks_hash_search(ctx->callbacks,
														   (const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE,
														   KS_UNLOCKED);
	ks_hash_read_unlock(ctx->callbacks);

	if (cb) cb(__get_sess_from_store_ctx(ctx), rqu, params);
}

static void __destroy_protocol(void *protocol)
{
	blade_protocol_t *proto = (blade_protocol_t *)protocol;
	BLADE_PROTOCOL_DESTROY(&proto);
}

static ks_status_t __add_protocol_obj(swclt_store_ctx_t *ctx, ks_json_t *obj)
{
	blade_protocol_t *protocol;
	ks_status_t status;

	if (status = BLADE_PROTOCOL_PARSE(ctx->base.pool, obj, &protocol)) {
		ks_log(KS_LOG_ERROR, "Failed to parse protocol: %d", status);
		return status;
	}


	if (status = ks_hash_insert(ctx->protocols, protocol->name, protocol)) {
		ks_log(KS_LOG_ERROR, "Failed to insert protocol: %d", status);
		BLADE_PROTOCOL_DESTROY(&protocol);
		return status;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __add_protocol_uncertified_obj(swclt_store_ctx_t *ctx, ks_json_t *obj)
{
	char *key;
	ks_status_t status;

	if (!ks_json_type_is_string(obj)) {
		status = KS_STATUS_ARG_INVALID;
		ks_log(KS_LOG_ERROR, "Failed to parse protocol uncertified: %d", status);
		return status;
	}

	key = ks_pstrdup(ks_pool_get(ctx->protocols_uncertified), ks_json_value_string(obj));

	if (status = ks_hash_insert(ctx->protocols_uncertified, key, (void *)KS_TRUE)) {
		ks_log(KS_LOG_ERROR, "Failed to insert protocol: %d", status);
		ks_pool_free(&key);
		return status;
	}

	return KS_STATUS_SUCCESS;
}

static void __destroy_node(void *node)
{
	blade_node_t *n = (blade_node_t *)node;
	BLADE_NODE_DESTROY(&n);
}

static ks_status_t __add_route_obj(swclt_store_ctx_t *ctx, ks_json_t *obj)
{
	blade_node_t *node;
	ks_status_t status;

	if (status = BLADE_NODE_PARSE(ctx->base.pool, obj, &node)) {
		ks_log(KS_LOG_ERROR, "Failed to parse route: %d", status);
		return status;
	}

	if (status = ks_hash_insert(ctx->routes, ks_pstrdup(ctx->base.pool, node->nodeid), node)) {
		ks_log(KS_LOG_ERROR, "Failed to insert route: %d", status);
		BLADE_NODE_DESTROY(&node);
		return status;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __lookup_protocol(swclt_store_ctx_t *ctx, const char *name, blade_protocol_t **protocol)
{
	blade_protocol_t *found_protocol = (blade_protocol_t *)
		ks_hash_search(ctx->protocols, name, KS_UNLOCKED);

	if (found_protocol) {
		if (protocol)
			*protocol = found_protocol;
		return KS_STATUS_SUCCESS;
	}

	return KS_STATUS_NOT_FOUND;
}

static ks_status_t __lookup_protocol_uncertified(swclt_store_ctx_t *ctx, const char *name)
{
	if (ks_hash_search(ctx->protocols_uncertified, name, KS_UNLOCKED) == NULL) return KS_STATUS_NOT_FOUND;
	return KS_STATUS_SUCCESS;
}

static void __remove_identities_by_nodeid(swclt_store_ctx_t *ctx, const char *nodeid)
{
	ks_hash_iterator_t *itt;
	ks_hash_t *cleanup = NULL;

	ks_hash_write_lock(ctx->identities);
	// iterate all identities
	for (itt = ks_hash_first(ctx->identities, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
		const char *val;
		const char *key;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&val);

		if (strcmp(nodeid, val)) continue;

		if (!cleanup)
			ks_hash_create(&cleanup, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_NOLOCK, ctx->base.pool);

		ks_log(KS_LOG_INFO, "Removing identity %s from node %s", key, val);

		ks_hash_insert(cleanup, key, (void *)KS_TRUE);
	}

    if (cleanup) {
		for (itt = ks_hash_first(cleanup, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
			const char *key = NULL;
			void *val = NULL;

			ks_hash_this(itt, (const void **)&key, NULL, (void **)&val);

			ks_hash_remove(ctx->identities, (const void *)key);
		}
		ks_hash_destroy(&cleanup);
	}
	
	ks_hash_write_unlock(ctx->identities);
}

static ks_status_t __get_node_identities(swclt_store_ctx_t *ctx,
										 const char *nodeid,
										 ks_pool_t *pool,
										 ks_hash_t **identities)
{
	ks_status_t status = KS_STATUS_SUCCESS;
    ks_hash_iterator_t *itt;

	ks_hash_create(identities, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_FREE_KEY, pool);

    ks_hash_read_lock(ctx->identities);
    // iterate all identities
    for (itt = ks_hash_first(ctx->identities, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
		const char *val;
		const char *key;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&val);

		if (strcmp(nodeid, val)) continue;

		ks_hash_insert(*identities, ks_pstrdup(pool, key), (void *)KS_TRUE);
	}

    ks_hash_read_unlock(ctx->identities);
}

static void __remove_provider_from_protocols(swclt_store_ctx_t *ctx, const char *nodeid)
{
	ks_hash_iterator_t *itt;
	ks_hash_t *cleanup = NULL;
	ks_json_t *entry = NULL;

	ks_log(KS_LOG_INFO, "Request to remove provider %s", nodeid);

	ks_hash_write_lock(ctx->protocols);
	// iterate all protocols
	for (itt = ks_hash_first(ctx->protocols, KS_UNLOCKED); itt; ) {
		blade_protocol_t *protocol;
		const char *key;
		int32_t index = 0;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&protocol);

		itt = ks_hash_next(&itt);

		ks_log(KS_LOG_INFO, "Iterating protocol %s with %lu providers",
			 protocol->name, ks_json_get_array_size(protocol->providers));

		// iterate protocol providers
		KS_JSON_ARRAY_FOREACH(entry, protocol->providers) {
			blade_provider_t *provider;
			ks_bool_t match = KS_FALSE;

			// parse provider
			ks_assertd(!BLADE_PROVIDER_PARSE(ctx->base.pool, entry, &provider));

			ks_log(KS_LOG_INFO, "Iterating provider %s", provider->nodeid);

			if (!strcmp(provider->nodeid, nodeid)) {
				match = KS_TRUE;

				ks_log(KS_LOG_INFO, "Removing provider %s from protocol %s", provider->nodeid, protocol->name);

				// delete provider from providers
				ks_json_delete_item_from_array(protocol->providers, index);

				if (ks_json_get_array_size(protocol->providers) == 0) {
					ks_log(KS_LOG_INFO, "No more providers present in protocol %s, removing", protocol->name);

					// Lazily init the cleanup hash which we'll walk if we decide to remove this at the end
					if (!cleanup)
						 ks_hash_create(&cleanup, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_NOLOCK, ctx->base.pool);

					// Just add the protocol no dupe her needed as the protocol and name are one item, we'll delete
					// the protocol which will free the key too when we iterate the cleanup hash below
					ks_hash_insert(cleanup, protocol->name, (void *)protocol);
				} else {
					ks_log(KS_LOG_INFO, "%lu providers remain in protocol %s, not removing", ks_json_get_array_size(protocol->providers), protocol->name);
				}
			}
			// cleanup parsed provider
			BLADE_PROVIDER_DESTROY(&provider);

			// if provider was found, stop searching
			if (match) break;
			++index;
		}
	}

	// cleanup protocols with no providers left
	if (cleanup) {
		for (itt = ks_hash_first(cleanup, KS_UNLOCKED); itt; ) {
			const char *key = NULL;
			blade_protocol_t *protocol = NULL;

			ks_hash_this(itt, (const void **)&key, NULL, (void **)&protocol);

			itt = ks_hash_next(&itt);

			ks_hash_remove(ctx->protocols, (const void *)key);
			__invoke_cb_protocol_remove(ctx, protocol->name);
		}
		ks_hash_destroy(&cleanup);
	}

	ks_hash_write_unlock(ctx->protocols);
}

static ks_status_t __select_random_protocol_provider(swclt_store_ctx_t *ctx, const char *name, ks_pool_t *pool, char **providerid)
{
	blade_protocol_t *protocol = NULL;
	int32_t count = 0;
	int32_t index = 0;
	ks_json_t *entry = NULL;
	blade_provider_t *provider = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;

	*providerid = NULL;

	ks_hash_write_lock(ctx->protocols);

	// find the protocol
	if (status = __lookup_protocol(ctx, name, &protocol))
		goto done;

	// get provider count
	if ((count = ks_json_get_array_size(protocol->providers)) == 0) {
		status = KS_STATUS_NOT_FOUND;
		goto done;
	}

	// pick random provider
	index = (int32_t)(rand() % ks_json_get_array_size(protocol->providers));
	// parse provider
	entry = ks_json_get_array_item(protocol->providers, index);
	ks_assertd(!BLADE_PROVIDER_PARSE(ctx->base.pool, entry, &provider));

	// copy provider id
	*providerid = ks_pstrdup(pool, provider->nodeid);

	// cleanup provider
	BLADE_PROVIDER_DESTROY(&provider);

done:
	ks_hash_write_unlock(ctx->protocols);

	return status;
}

static ks_status_t __get_protocols(swclt_store_ctx_t *ctx,
								   ks_pool_t *pool,
								   ks_json_t **protocols)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	*protocols = ks_json_pcreate_array(pool);

	ks_hash_read_lock(ctx->protocols);

	for (ks_hash_iterator_t *it = ks_hash_first(ctx->protocols, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
		const char *key = NULL;
		blade_protocol_t *proto = NULL;

		ks_hash_this(it, (const void **)&key, NULL, (void **)&proto);

		ks_json_padd_string_to_array(pool, *protocols, key);
	}

	ks_hash_read_unlock(ctx->protocols);
	return status;
}

static ks_status_t __get_protocol_providers(swclt_store_ctx_t *ctx,
											const char *name,
											ks_pool_t *pool,
											ks_json_t **providers)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	blade_protocol_t *protocol = NULL;

	*providers = NULL;

	ks_hash_read_lock(ctx->protocols);

	if (status = __lookup_protocol(ctx, name, &protocol))
		goto done;

	if (ks_json_get_array_size(protocol->providers) == 0) {
		status = KS_STATUS_NOT_FOUND;
		goto done;
	}

	*providers = ks_json_pduplicate(pool, protocol->providers, KS_TRUE);

done:
	ks_hash_read_unlock(ctx->protocols);
	return status;
}

static ks_status_t __add_authority_obj(swclt_store_ctx_t *ctx, ks_json_t *obj)
{
	blade_netcast_authority_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_AUTHORITY_ADD_PARAM_PARSE(ctx->base.pool, obj, &params)) {
		ks_log(KS_LOG_ERROR, "Failed to parse authority: %d", status);
		return status;
	}

	if (status = ks_hash_insert(ctx->authorities, ks_pstrdup(ctx->base.pool, params->nodeid), (void *)KS_TRUE)) {
		ks_log(KS_LOG_ERROR, "Failed to insert authority: %d", status);
		BLADE_NETCAST_AUTHORITY_ADD_PARAM_DESTROY(&params);
		return status;
	}
	
	BLADE_NETCAST_AUTHORITY_ADD_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Protocol add/remove for uncertified clients only
static ks_status_t __update_protocol_add(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_add_param_t *params = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;

	if (status = BLADE_NETCAST_PROTOCOL_ADD_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	ks_log(KS_LOG_INFO, "Request to add new protocol %s", params->protocol);

	/* Lookup the protocol */
	if (status = __lookup_protocol_uncertified(ctx, params->protocol)) {
		char *key = ks_pstrdup(ks_pool_get(ctx->protocols_uncertified), params->protocol);
		
		ks_log(KS_LOG_INFO, "Protocol %s does not exist yet, adding new entry", params->protocol);

		/* And add it */
		if (status = ks_hash_insert(ctx->protocols_uncertified, key, (void *)KS_TRUE)) {
			ks_pool_free(&key);
			BLADE_NETCAST_PROTOCOL_ADD_PARAM_DESTROY(&params);
			return status;
		}

		ks_log(KS_LOG_INFO, "Protocol %s added", params->protocol);

		__invoke_cb_protocol_add(ctx, params->protocol);

		return status;

	}

	ks_log(KS_LOG_INFO, "Protocol %s exists already", params->protocol);

	BLADE_NETCAST_PROTOCOL_ADD_PARAM_DESTROY(&params);
	
	return status;
}

static ks_status_t __update_protocol_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_remove_param_t *params = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t match = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_REMOVE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	match = ks_hash_remove(ctx->protocols_uncertified, (const void *)params->protocol) != NULL;

done:

	if (match) __invoke_cb_protocol_remove(ctx, params->protocol);

	BLADE_NETCAST_PROTOCOL_REMOVE_PARAM_DESTROY(&params);

	return status;
}

// Provider add/remove
static ks_status_t __update_protocol_provider_add(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_add_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_json_t *provider_data = NULL;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	ks_log(KS_LOG_INFO, "Request to add new provider %s for protocol %s", params->nodeid, params->protocol);

	ks_hash_write_lock(ctx->protocols);

	/* Lookup the protocol */
	if (status = __lookup_protocol(ctx, params->protocol, &protocol)) {
		ks_log(KS_LOG_INFO, "Protocol %s does not exist yet, adding new entry", params->protocol);

		/* Gotta add a new one */
		if (!(protocol = ks_pool_alloc(ctx->base.pool, sizeof(blade_protocol_t)))) {
			ks_hash_write_unlock(ctx->protocols);
			return KS_STATUS_NO_MEM;
		}

		protocol->channels = ks_json_pduplicate(ctx->base.pool, params->channels, KS_TRUE);

		protocol->default_channel_broadcast_access = params->default_channel_broadcast_access;
		protocol->default_channel_subscribe_access = params->default_channel_subscribe_access;
		protocol->default_method_execute_access = params->default_method_execute_access;

		protocol->name = ks_pstrdup(ctx->base.pool, params->protocol);

		if (params->data) {
			provider_data = ks_json_pduplicate(ctx->base.pool, params->data, KS_TRUE);
		}
		if (!(protocol->providers = ks_json_pcreate_array_inline(ctx->base.pool, 1,
																										BLADE_PROVIDER_MARSHAL(ctx->base.pool,
																										   &(blade_provider_t){params->nodeid, NULL, params->rank, provider_data})))) {
			ks_pool_free(&protocol);
			ks_pool_free(&provider_data);
			BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);
			ks_hash_write_unlock(ctx->protocols);
			return KS_STATUS_NO_MEM;
		}

		/* And add it */
		if (status = ks_hash_insert(ctx->protocols, protocol->name, protocol)) {
			ks_pool_free(&protocol);
			BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);
			ks_hash_write_unlock(ctx->protocols);
			return status;

		}

		ks_log(KS_LOG_INFO, "Protocol %s added with provider %s", protocol->name, params->nodeid);

		// @todo come back and fix this, params->protocol is going to be NULL, need to duplicate
		// the string so it remains valid in params
		__invoke_cb_protocol_add(ctx, params->protocol);
		__invoke_cb_protocol_provider_add(ctx, netcast_rqu, params);

		ks_hash_write_unlock(ctx->protocols);

		return status;

	}

	ks_log(KS_LOG_INFO, "Protocol %s exists already, has %lu providers currently",
				params->protocol, ks_json_get_array_size(protocol->providers));

	/* Now add any provider entries to the protocol */
	if (params->data) {
		provider_data = ks_json_pduplicate(ctx->base.pool, params->data, KS_TRUE);
	}
	if (!ks_json_add_item_to_array(protocol->providers,
					BLADE_PROVIDER_MARSHAL(ctx->base.pool, &(blade_provider_t){params->nodeid, NULL, params->rank, provider_data}))) {
		ks_pool_free(&provider_data);
		ks_hash_write_unlock(ctx->protocols);
		return KS_STATUS_NO_MEM;
	}

	ks_log(KS_LOG_INFO, "Protocol %s add complete, provider count %lu", protocol->name, ks_json_get_array_size(protocol->providers));

	__invoke_cb_protocol_provider_add(ctx, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);

	ks_hash_write_unlock(ctx->protocols);
	
	return status;
}

static ks_status_t __update_protocol_provider_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_remove_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_json_t *entry = NULL;
	int32_t index = 0;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t match = KS_FALSE;
	ks_bool_t cleanup = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_REMOVE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(ctx->protocols);

	if (status = __lookup_protocol(ctx, params->protocol, &protocol)) {
		ks_log(KS_LOG_WARNING, "Received protocol provider remove for protocol '%s' which does not exist", params->protocol);
		status = KS_STATUS_SUCCESS;
		goto done;
	}

		// iterate protocol providers
	KS_JSON_ARRAY_FOREACH(entry, protocol->providers) {
		blade_provider_t *provider;

		// parse provider
		ks_assertd(!BLADE_PROVIDER_PARSE(ctx->base.pool, entry, &provider));

		if (!strcmp(provider->nodeid, params->nodeid)) {
			match = KS_TRUE;

			// delete provider from providers
			ks_json_delete_item_from_array(protocol->providers, index);

			cleanup = ks_json_get_array_size(protocol->providers) == 0;
		}
		// cleanup parsed provider
		BLADE_PROVIDER_DESTROY(&provider);

		// if provider was found, stop searching
		if (match) break;
		++index;
	}

	if (cleanup) {
		// cleanup protocol if no providers left
		ks_hash_remove(ctx->protocols, (const void *)protocol->name);
		__invoke_cb_protocol_remove(ctx, protocol->name);
	}

done:
	ks_hash_write_unlock(ctx->protocols);

	if (match) __invoke_cb_protocol_provider_remove(ctx, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_REMOVE_PARAM_DESTROY(&params);

	return status;
}

// Provider rank update
static ks_status_t __update_protocol_provider_rank_update(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_rank_update_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_json_t *entry = NULL;
	blade_provider_t *provider = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t found = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(ctx->protocols);

	ks_log(KS_LOG_INFO, "Request to update rank for provider %s to %d for protocol %s", params->nodeid, params->rank, params->protocol);

	/* Lookup the protocol */
	if (status = __lookup_protocol(ctx, params->protocol, &protocol))
		goto done;

	// find provider
	for (int32_t index = 0; index < ks_json_get_array_size(protocol->providers); ++index) {
		entry = ks_json_get_array_item(protocol->providers, index);

		const char *provider_nodeid = ks_json_get_object_cstr_def(entry, "nodeid", "");
		if (!strcmp(provider_nodeid, params->nodeid)) {
			found = KS_TRUE;
			ks_json_delete_item_from_object(entry, "rank");
			ks_json_padd_number_to_object(ctx->base.pool, entry, "rank", params->rank);
		}
	}

done:
	ks_hash_write_unlock(ctx->protocols);

	if (found) __invoke_cb_protocol_provider_rank_update(ctx, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM_DESTROY(&params);
	
	return status;
}

// Provider data update
static ks_status_t __update_protocol_provider_data_update(swclt_store_ctx_t *ctx, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_data_update_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_json_t *entry = NULL;
	blade_provider_t *provider = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t found = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_DATA_UPDATE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(ctx->protocols);

	ks_log(KS_LOG_INFO, "Request to update data for provider %s for protocol %s", params->nodeid, params->protocol);

	/* Lookup the protocol */
	if (status = __lookup_protocol(ctx, params->protocol, &protocol))
		goto done;

	// find provider
	for (int32_t index = 0; index < ks_json_get_array_size(protocol->providers); ++index) {
		entry = ks_json_get_array_item(protocol->providers, index);
		const char *provider_nodeid = ks_json_get_object_cstr_def(entry, "nodeid", "");
		if (!strcmp(provider_nodeid, params->nodeid)) {
			found = KS_TRUE;
			ks_json_delete_item_from_object(entry, "data");
			ks_json_add_item_to_object(entry, "data", ks_json_pduplicate(ctx->base.pool, params->data, KS_TRUE));
		}
	}

done:
	ks_hash_write_unlock(ctx->protocols);

	if (found) __invoke_cb_protocol_provider_data_update(ctx, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_DATA_UPDATE_PARAM_DESTROY(&params);
	
	return status;
}


// Route add/remove
static ks_status_t __update_route_add(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_route_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_ROUTE_ADD_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	status = __add_route_obj(ctx, netcast_rqu->params);

	__invoke_cb_route_add(ctx, netcast_rqu, params);

	BLADE_NETCAST_ROUTE_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_route_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_route_remove_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_ROUTE_REMOVE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	__invoke_cb_route_remove(ctx, netcast_rqu, params);

	/* Remove the node from the hash */
	ks_hash_remove(ctx->routes, params->nodeid);

	/* Remove the identities that map to the nodeid */
	__remove_identities_by_nodeid(ctx, params->nodeid);

	/* Now we have to rummage through the protocols hash and remove any protocols hosted by this node */
	__remove_provider_from_protocols(ctx, params->nodeid);

	// @todo purge subscribers with the given nodeid
	
	ks_hash_remove(ctx->authorities, params->nodeid);
	//ks_hash_remove(ctx->accesses, params->nodeid);

	/* Done with the request */
	BLADE_NETCAST_ROUTE_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Identity add/remove
static ks_status_t __update_identity_add(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_identity_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_IDENTITY_ADD_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

    if (status = ks_hash_insert(ctx->identities, ks_pstrdup(ctx->base.pool, params->identity), ks_pstrdup(ctx->base.pool, params->nodeid))) {
		ks_log(KS_LOG_ERROR, "Failed to insert identity: %d", status);
		goto done;
	}
	
	__invoke_cb_identity_add(ctx, netcast_rqu, params);

done:
	BLADE_NETCAST_IDENTITY_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_identity_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_identity_remove_param_t *params;
	ks_status_t status;
	const char *nodeid = NULL;

	if (status = BLADE_NETCAST_IDENTITY_REMOVE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(ctx->identities);

	/* Make sure the identity is owned by the right nodeid */
	nodeid = (const char *)ks_hash_search(ctx->identities, params->identity, KS_UNLOCKED);
	if (nodeid && !strcmp(nodeid, params->nodeid))
	{
		/* Remove the identity from the hash */
		__invoke_cb_identity_remove(ctx, netcast_rqu, params);
		ks_hash_remove(ctx->identities, params->identity);
	}

	ks_hash_write_unlock(ctx->identities);

	/* Done with the request */
	BLADE_NETCAST_IDENTITY_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Channel add/remove
static ks_status_t __update_channel_add(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update_channel_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

// Subscription add/remove
static ks_status_t __update_subscription_add(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_subscription_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_SUBSCRIPTION_ADD_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	/* @TODO add subscription to ctx->subscriptions */

	__invoke_cb_subscription_add(ctx, netcast_rqu, params);

	BLADE_NETCAST_SUBSCRIPTION_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_subscription_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_subscription_remove_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_SUBSCRIPTION_REMOVE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	/* @TODO Remove the subscription from ctx->subscriptions */

	__invoke_cb_subscription_remove(ctx, netcast_rqu, params);

	/* Done with the request */
	BLADE_NETCAST_SUBSCRIPTION_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Authorization add/remove
static ks_status_t __update_authorization_add(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update_authorization_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

// Authority add/remove
static ks_status_t __update_authority_add(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_authority_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_AUTHORITY_ADD_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	status = __add_authority_obj(ctx, netcast_rqu->params);

	__invoke_cb_authority_add(ctx, netcast_rqu, params);

	BLADE_NETCAST_AUTHORITY_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_authority_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_authority_remove_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_AUTHORITY_REMOVE_PARAM_PARSE(ctx->base.pool, netcast_rqu->params, &params))
		return status;

	/* Remove the node from the hash */
	ks_hash_remove(ctx->authorities, params->nodeid);

	__invoke_cb_authority_remove(ctx, netcast_rqu, params);

	/* Done with the request */
	BLADE_NETCAST_AUTHORITY_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Access add/remove
static ks_status_t __update_access_add(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update_access_remove(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update(swclt_store_ctx_t *ctx, blade_netcast_rqu_t *netcast_rqu)
{
	/* Based on the command, do the appropriate update call */
	if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_ADD))
		return __update_protocol_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_REMOVE))
		return __update_protocol_remove(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD))
		return __update_protocol_provider_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE))
		return __update_protocol_provider_remove(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE))
		return __update_protocol_provider_rank_update(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE))
		return __update_protocol_provider_data_update(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ROUTE_ADD))
		return __update_route_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ROUTE_REMOVE))
		return __update_route_remove(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_IDENTITY_ADD))
		return __update_identity_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_IDENTITY_REMOVE))
		return __update_identity_remove(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_SUBSCRIPTION_ADD))
		return __update_subscription_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE))
		return __update_subscription_remove(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORITY_ADD))
		return __update_authority_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORITY_REMOVE))
		return __update_authority_remove(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORIZATION_ADD))
		return __update_authorization_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORIZATION_REMOVE))
		return __update_authorization_remove(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ACCESS_ADD))
		return __update_access_add(ctx, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ACCESS_REMOVE))
		return __update_access_remove(ctx, netcast_rqu);
	else {
		ks_log(KS_LOG_WARNING, "Unknown netcast subcommand: %s", netcast_rqu->command);
		return KS_STATUS_SUCCESS;
	}
}

static ks_status_t __populate_routes(swclt_store_ctx_t *ctx, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;
	const char *nodeid;
	ks_json_t *identities;

	/* Walk the routes array and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->routes) {
		if (status = __add_route_obj(ctx, entry)) {
			ks_log(KS_LOG_ERROR, "Failed to populate route: %d", status);
			return status;
		}

		// Identities
		nodeid = ks_json_get_object_cstr(entry, "nodeid");
		identities = ks_json_get_object_item(entry, "identities");
		if (nodeid && identities && ks_json_type_is_array(identities))
		{
			int size = ks_json_get_array_size(identities);
			for (int index = 0; index < size; ++index)
			{
				const char *identity = ks_json_get_array_cstr(identities, index);
				if (!identity) continue;
				ks_hash_insert(ctx->identities, ks_pstrdup(ctx->base.pool, identity), ks_pstrdup(ctx->base.pool, nodeid));
			}
		}
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __populate_protocols(swclt_store_ctx_t *ctx, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->protocols) {
		if (status = __add_protocol_obj(ctx, entry)) {
			ks_log(KS_LOG_ERROR, "Failed to populate protocol: %d", status);
			return status;
		}
	}

	return KS_STATUS_SUCCESS;
}

static void __destroy_subscription(void *subscription)
{
	blade_subscription_t *sub = (blade_subscription_t *)subscription;
	BLADE_SUBSCRIPTION_DESTROY(&sub);
}

static ks_status_t __populate_subscriptions(swclt_store_ctx_t *ctx, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->subscriptions) {
		blade_subscription_t *subscription;

		if (status = BLADE_SUBSCRIPTION_PARSE(ctx->base.pool, entry, &subscription)) {
			ks_log(KS_LOG_ERROR, "Failed to parse subscription: %d", status);
			return status;
		}

		/* Subscriptions get keyed by a combo key with the channel/protocol */
		if (status = ks_hash_insert(ctx->subscriptions, ks_psprintf(ctx->base.pool, "%s:%s", subscription->protocol, subscription->channel), subscription)) {
			ks_log(KS_LOG_ERROR, "Failed to insert subscription: %d", status);
			BLADE_SUBSCRIPTION_DESTROY(&subscription);
			return status;
		}
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __populate_authorities(swclt_store_ctx_t *ctx, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->authorities) {
		const char *authority = ks_json_value_string(entry);
		authority = ks_pstrdup(ks_pool_get(ctx->authorities), authority);

		if (status = ks_hash_insert(ctx->authorities, authority, (void *)KS_TRUE)) {
			ks_log(KS_LOG_ERROR, "Failed to insert authority: %d", status);
			ks_pool_free(&authority);
			return status;
		}
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __populate_protocols_uncertified(swclt_store_ctx_t *ctx, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->protocols_uncertified) {
		if (status = __add_protocol_uncertified_obj(ctx, entry)) {
			ks_log(KS_LOG_ERROR, "Failed to populate protocol for uncertified client: %d", status);
			return status;
		}
	}

	return KS_STATUS_SUCCESS;
}


static void __context_describe(swclt_store_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	snprintf(buffer, buffer_len, "SWCLT Store: protocols %d routes %d authorities: %d subscriptions: %d",
		ks_hash_count(ctx->protocols), ks_hash_count(ctx->routes), ks_hash_count(ctx->authorities), ks_hash_count(ctx->subscriptions));
}

static void __context_deinit(swclt_store_ctx_t *ctx)
{
	ks_hash_destroy(&ctx->callbacks);
	ks_hash_destroy(&ctx->subscriptions);
	ks_hash_destroy(&ctx->protocols);
	ks_hash_destroy(&ctx->identities);
	ks_hash_destroy(&ctx->routes);
	ks_hash_destroy(&ctx->authorities);
	ks_hash_destroy(&ctx->protocols_uncertified);
}

static ks_status_t __context_init(swclt_store_ctx_t *ctx)
{
	ks_status_t status;

	if (status = ks_hash_create(
			&ctx->callbacks,
			KS_HASH_MODE_INT,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_RWLOCK,
			ctx->base.pool))
		return status;

	/* Create our authorities hash, keyed by the node id of the authority */
	if (status = ks_hash_create(
			&ctx->authorities,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			ctx->base.pool))
		return status;

	/* Create our subscriptions hash, keyed by the protocol + channel name */
	if (status = ks_hash_create(
			&ctx->subscriptions,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			ctx->base.pool))
		return status;
	ks_hash_set_destructor(ctx->subscriptions, __destroy_subscription);

	/* Create our protocols hash, keyed by the protocol name */
	if (status = ks_hash_create(
			&ctx->protocols,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK,  /* the key is inside the value struct - only need to destroy value */
			ctx->base.pool))
		return status;
	ks_hash_set_destructor(ctx->protocols, __destroy_protocol);

	/* Create our routes hash, keyed by the nodeid */
	if (status = ks_hash_create(
			&ctx->routes,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			ctx->base.pool))
		return status;
	ks_hash_set_destructor(ctx->routes, __destroy_node);

	/* Create our identities hash, keyed by the identity */
	if (status = ks_hash_create(
			&ctx->identities,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE,
			ctx->base.pool))
		return status;

	/* Create our uncertified client protocols hash, keyed by the protocol name */
	if (status = ks_hash_create(
			&ctx->protocols_uncertified,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			ctx->base.pool))
		return status;

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_store_reset(swclt_store_t store)
{
	ks_hash_iterator_t *itt;

	SWCLT_STORE_SCOPE_BEG(store, ctx, status)

	while (itt = ks_hash_first(ctx->routes, KS_UNLOCKED)) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		ks_hash_remove(ctx->routes, (const void *)key);
	}

	while (itt = ks_hash_first(ctx->identities, KS_UNLOCKED)) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		ks_hash_remove(ctx->identities, (const void *)key);
	}

	while (itt = ks_hash_first(ctx->protocols, KS_UNLOCKED)) {
		const char *key = NULL;
		blade_protocol_t *protocol = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&protocol);

		ks_hash_remove(ctx->protocols, (const void *)key);
	}

	while (itt = ks_hash_first(ctx->subscriptions, KS_UNLOCKED)) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		ks_hash_remove(ctx->subscriptions, (const void *)key);
	}


	while (itt = ks_hash_first(ctx->authorities, KS_UNLOCKED)) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		ks_hash_remove(ctx->authorities, (const void *)key);
	}

	while (itt = ks_hash_first(ctx->protocols_uncertified, KS_UNLOCKED)) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		ks_hash_remove(ctx->protocols_uncertified, (const void *)key);
	}


	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_populate(swclt_store_t store, const blade_connect_rpl_t *connect_rpl)
{
	blade_connect_rpl_t *rpl = (blade_connect_rpl_t *)connect_rpl;
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)

	/* Now popualte the fields from the connect result */
	if (status = __populate_routes(ctx, rpl))
		return status;

	if (status = __populate_protocols(ctx, rpl))
		return status;

	if (status = __populate_subscriptions(ctx, rpl))
		return status;

	if (status = __populate_authorities(ctx, rpl))
		return status;

	if (status = __populate_protocols_uncertified(ctx, rpl))
		return status;

	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_create(swclt_store_t *store)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_S(
		NULL,
		SWCLT_HTYPE_STORE,
		store,
		swclt_store_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init)
}

SWCLT_DECLARE(ks_status_t) swclt_store_get_node_identities(swclt_store_t store,
														   const char *nodeid,
														   ks_pool_t *pool,
														   ks_hash_t **identities)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __get_node_identities(ctx, nodeid, pool, identities);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_get_protocols(swclt_store_t store, ks_pool_t *pool, ks_json_t **protocols)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __get_protocols(ctx, pool, protocols);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

/* Returns success if the protocol exists and the store is valid, also works for uncertified client protocols check. */
SWCLT_DECLARE(ks_status_t) swclt_store_check_protocol(swclt_store_t store, const char *name)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	if (status = __lookup_protocol(ctx, name, NULL)) {
		status = __lookup_protocol_uncertified(ctx, name);
	}
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_select_random_protocol_provider(
	swclt_store_t store,
   	const char *name,
	ks_pool_t *pool,
	char **providerid)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __select_random_protocol_provider(ctx, name, pool, providerid);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_get_protocol_providers(swclt_store_t store,
															  const char *name,
															  ks_pool_t *pool,
															  ks_json_t **providers)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __get_protocol_providers(ctx, name, pool, providers);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_update(swclt_store_t store, const blade_netcast_rqu_t *netcast_rqu)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __update(ctx, (blade_netcast_rqu_t *) netcast_rqu);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_route_add(swclt_store_t store, swclt_store_cb_route_add_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_route_add(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_route_remove(swclt_store_t store, swclt_store_cb_route_remove_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_route_remove(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_identity_add(swclt_store_t store, swclt_store_cb_identity_add_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_identity_add(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_identity_remove(swclt_store_t store, swclt_store_cb_identity_remove_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_identity_remove(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_add(swclt_store_t store, swclt_store_cb_protocol_add_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_protocol_add(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_remove(swclt_store_t store, swclt_store_cb_protocol_remove_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_protocol_remove(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_add(swclt_store_t store, swclt_store_cb_protocol_provider_add_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_protocol_provider_add(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_remove(swclt_store_t store, swclt_store_cb_protocol_provider_remove_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_protocol_provider_remove(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_rank_update(swclt_store_t store, swclt_store_cb_protocol_provider_rank_update_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_protocol_provider_rank_update(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_data_update(swclt_store_t store, swclt_store_cb_protocol_provider_data_update_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_protocol_provider_data_update(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_authority_add(swclt_store_t store, swclt_store_cb_authority_add_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_authority_add(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_authority_remove(swclt_store_t store, swclt_store_cb_authority_remove_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_authority_remove(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_subscription_add(swclt_store_t store, swclt_store_cb_subscription_add_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_subscription_add(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_subscription_remove(swclt_store_t store, swclt_store_cb_subscription_remove_t cb)
{
	SWCLT_STORE_SCOPE_BEG(store, ctx, status)
	status = __add_cb_subscription_remove(ctx, cb);
	SWCLT_STORE_SCOPE_END(store, ctx, status)
}
