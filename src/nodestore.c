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


static ks_status_t __add_cb_route_add(swclt_store_t *store, swclt_store_cb_route_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding route add handler for method: %s", BLADE_NETCAST_CMD_ROUTE_ADD);
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_ROUTE_ADD, (void *)cb);
}

static void __invoke_cb_route_add(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_route_add_param_t *params)
{
	swclt_store_cb_route_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up route add handler for method: %s", BLADE_NETCAST_CMD_ROUTE_ADD);

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_route_add_t)ks_hash_search(store->callbacks,
													(const void *)BLADE_NETCAST_CMD_ROUTE_ADD,
													KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store add");
		cb(store->sess, rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for route add method: %s", BLADE_NETCAST_CMD_ROUTE_ADD);
	}
}

static ks_status_t __add_cb_route_remove(swclt_store_t *store, swclt_store_cb_route_remove_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_ROUTE_REMOVE, (void *)cb);
}

static void __invoke_cb_route_remove(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_route_remove_param_t *params)
{
	swclt_store_cb_route_remove_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_route_remove_t)ks_hash_search(store->callbacks,
													   (const void *)BLADE_NETCAST_CMD_ROUTE_REMOVE,
													   KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static ks_status_t __add_cb_identity_add(swclt_store_t *store, swclt_store_cb_identity_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding identity add handler for method: %s", BLADE_NETCAST_CMD_IDENTITY_ADD);
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_IDENTITY_ADD, (void *)cb);
}

static void __invoke_cb_identity_add(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_identity_add_param_t *params)
{
	swclt_store_cb_identity_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up identity add handler for method: %s", BLADE_NETCAST_CMD_IDENTITY_ADD);

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_identity_add_t)ks_hash_search(store->callbacks,
													   (const void *)BLADE_NETCAST_CMD_IDENTITY_ADD,
													   KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store identity add");
		cb(store->sess, rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for identity add method: %s", BLADE_NETCAST_CMD_IDENTITY_ADD);
	}
}

static ks_status_t __add_cb_identity_remove(swclt_store_t *store, swclt_store_cb_identity_remove_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_IDENTITY_REMOVE, (void *)cb);
}

static void __invoke_cb_identity_remove(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_identity_remove_param_t *params)
{
	swclt_store_cb_identity_remove_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_identity_remove_t)ks_hash_search(store->callbacks,
														  (const void *)BLADE_NETCAST_CMD_IDENTITY_REMOVE,
														  KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static ks_status_t __add_cb_protocol_add(swclt_store_t *store, swclt_store_cb_protocol_add_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_ADD, (void *)cb);
}

static void __invoke_cb_protocol_add(swclt_store_t *store,
									 const char *protocol)
{
	swclt_store_cb_protocol_add_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_protocol_add_t)ks_hash_search(store->callbacks,
													   (const void *)BLADE_NETCAST_CMD_PROTOCOL_ADD,
													   KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, protocol);
}

static ks_status_t __add_cb_protocol_remove(swclt_store_t *store, swclt_store_cb_protocol_remove_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_REMOVE, (void *)cb);
}

static void __invoke_cb_protocol_remove(swclt_store_t *store,
										const char *protocol)
{
	swclt_store_cb_protocol_remove_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_protocol_remove_t)ks_hash_search(store->callbacks,
														  (const void *)BLADE_NETCAST_CMD_PROTOCOL_REMOVE,
														  KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, protocol);
}

static ks_status_t __add_cb_protocol_provider_add(swclt_store_t *store, swclt_store_cb_protocol_provider_add_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD, (void *)cb);
}

static void __invoke_cb_protocol_provider_add(swclt_store_t *store,
											  const blade_netcast_rqu_t *rqu,
											  const blade_netcast_protocol_provider_add_param_t *params)
{
	swclt_store_cb_protocol_provider_add_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_protocol_provider_add_t)ks_hash_search(store->callbacks,
																(const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD,
																KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static ks_status_t __add_cb_protocol_provider_remove(swclt_store_t *store, swclt_store_cb_protocol_provider_remove_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE, (void *)cb);
}

static void __invoke_cb_protocol_provider_remove(swclt_store_t *store,
												 const blade_netcast_rqu_t *rqu,
												 const blade_netcast_protocol_provider_remove_param_t *params)
{
	swclt_store_cb_protocol_provider_remove_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_protocol_provider_remove_t)ks_hash_search(store->callbacks,
																   (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE,
																   KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static ks_status_t __add_cb_protocol_provider_rank_update(swclt_store_t *store, swclt_store_cb_protocol_provider_rank_update_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE, (void *)cb);
}

static void __invoke_cb_protocol_provider_rank_update(swclt_store_t *store,
													  const blade_netcast_rqu_t *rqu,
													  const blade_netcast_protocol_provider_rank_update_param_t *params)
{
	swclt_store_cb_protocol_provider_rank_update_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_protocol_provider_rank_update_t)ks_hash_search(store->callbacks,
																		(const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE,
																		KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static ks_status_t __add_cb_protocol_provider_data_update(swclt_store_t *store, swclt_store_cb_protocol_provider_data_update_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE, (void *)cb);
}

static void __invoke_cb_protocol_provider_data_update(swclt_store_t *store,
													  const blade_netcast_rqu_t *rqu,
													  const blade_netcast_protocol_provider_data_update_param_t *params)
{
	swclt_store_cb_protocol_provider_data_update_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_protocol_provider_data_update_t)ks_hash_search(store->callbacks,
																		(const void *)BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE,
																		KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static ks_status_t __add_cb_authority_add(swclt_store_t *store, swclt_store_cb_authority_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding authority add handler for method: %s", BLADE_NETCAST_CMD_AUTHORITY_ADD);
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_AUTHORITY_ADD, (void *)cb);
}

static void __invoke_cb_authority_add(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_authority_add_param_t *params)
{
	swclt_store_cb_authority_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up authority add handler for method: %s", BLADE_NETCAST_CMD_AUTHORITY_ADD);

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_authority_add_t)ks_hash_search(store->callbacks,
														(const void *)BLADE_NETCAST_CMD_AUTHORITY_ADD,
														KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store add");
		cb(store->sess, rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for authority add method: %s", BLADE_NETCAST_CMD_AUTHORITY_ADD);
	}
}

static ks_status_t __add_cb_authority_remove(swclt_store_t *store, swclt_store_cb_authority_remove_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_AUTHORITY_REMOVE, (void *)cb);
}

static void __invoke_cb_authority_remove(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_authority_remove_param_t *params)
{
	swclt_store_cb_authority_remove_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_authority_remove_t)ks_hash_search(store->callbacks,
														   (const void *)BLADE_NETCAST_CMD_AUTHORITY_REMOVE,
														   KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static ks_status_t __add_cb_subscription_add(swclt_store_t *store, swclt_store_cb_subscription_add_t cb)
{
	ks_log(KS_LOG_DEBUG, "Adding authority add handler for method: %s", BLADE_NETCAST_CMD_SUBSCRIPTION_ADD);
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_ADD, (void *)cb);
}

static void __invoke_cb_subscription_add(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_subscription_add_param_t *params)
{
	swclt_store_cb_subscription_add_t cb;

	ks_log(KS_LOG_DEBUG, "Looking up subscription add handler for method: %s", BLADE_NETCAST_CMD_SUBSCRIPTION_ADD);

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_subscription_add_t)ks_hash_search(store->callbacks,
														(const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_ADD,
														KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);


	if (cb) {
		ks_log(KS_LOG_DEBUG, "Invoking callback for node store add");
		cb(store->sess, rqu, params);
	} else {
		ks_log(KS_LOG_DEBUG, "No callback registered for subscription add method: %s", BLADE_NETCAST_CMD_SUBSCRIPTION_ADD);
	}
}

static ks_status_t __add_cb_subscription_remove(swclt_store_t *store, swclt_store_cb_subscription_remove_t cb)
{
	return ks_hash_insert(store->callbacks, (const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE, (void *)cb);
}

static void __invoke_cb_subscription_remove(swclt_store_t *store, const blade_netcast_rqu_t *rqu, const blade_netcast_subscription_remove_param_t *params)
{
	swclt_store_cb_subscription_remove_t cb;

	ks_hash_read_lock(store->callbacks);
	cb = (swclt_store_cb_subscription_remove_t)ks_hash_search(store->callbacks,
														   (const void *)BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE,
														   KS_UNLOCKED);
	ks_hash_read_unlock(store->callbacks);

	if (cb) cb(store->sess, rqu, params);
}

static void __destroy_protocol(void *protocol)
{
	blade_protocol_t *proto = (blade_protocol_t *)protocol;
	BLADE_PROTOCOL_DESTROY(&proto);
}

static ks_status_t __add_protocol_obj(swclt_store_t *store, ks_json_t *obj)
{
	blade_protocol_t *protocol;
	ks_status_t status;

	if (status = BLADE_PROTOCOL_PARSE(store->pool, obj, &protocol)) {
		ks_log(KS_LOG_ERROR, "Failed to parse protocol: %d", status);
		return status;
	}


	if (status = ks_hash_insert(store->protocols, protocol->name, protocol)) {
		ks_log(KS_LOG_ERROR, "Failed to insert protocol: %d", status);
		BLADE_PROTOCOL_DESTROY(&protocol);
		return status;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __add_protocol_uncertified_obj(swclt_store_t *store, ks_json_t *obj)
{
	char *key;
	ks_status_t status;

	if (!ks_json_type_is_string(obj)) {
		status = KS_STATUS_ARG_INVALID;
		ks_log(KS_LOG_ERROR, "Failed to parse protocol uncertified: %d", status);
		return status;
	}
	const char *str;
	ks_json_value_string(obj, &str);
	key = ks_pstrdup(ks_pool_get(store->protocols_uncertified), str);

	if (status = ks_hash_insert(store->protocols_uncertified, key, (void *)KS_TRUE)) {
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

static ks_status_t __add_route_obj(swclt_store_t *store, ks_json_t *obj)
{
	blade_node_t *node;
	ks_status_t status;

	if (status = BLADE_NODE_PARSE(store->pool, obj, &node)) {
		ks_log(KS_LOG_ERROR, "Failed to parse route: %d", status);
		return status;
	}

	if (status = ks_hash_insert(store->routes, ks_pstrdup(store->pool, node->nodeid), node)) {
		ks_log(KS_LOG_ERROR, "Failed to insert route: %d", status);
		BLADE_NODE_DESTROY(&node);
		return status;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __lookup_protocol(swclt_store_t *store, const char *name, blade_protocol_t **protocol)
{
	blade_protocol_t *found_protocol = (blade_protocol_t *)
		ks_hash_search(store->protocols, name, KS_UNLOCKED);

	if (found_protocol) {
		if (protocol)
			*protocol = found_protocol;
		return KS_STATUS_SUCCESS;
	}

	return KS_STATUS_NOT_FOUND;
}

static ks_status_t __lookup_protocol_uncertified(swclt_store_t *store, const char *name)
{
	if (ks_hash_search(store->protocols_uncertified, name, KS_UNLOCKED) == NULL) return KS_STATUS_NOT_FOUND;
	return KS_STATUS_SUCCESS;
}

static void __remove_identities_by_nodeid(swclt_store_t *store, const char *nodeid)
{
	ks_hash_iterator_t *itt;
	ks_hash_t *cleanup = NULL;

	ks_hash_write_lock(store->identities);
	// iterate all identities
	for (itt = ks_hash_first(store->identities, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
		const char *val;
		const char *key;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&val);

		if (strcmp(nodeid, val)) continue;

		if (!cleanup)
			ks_hash_create(&cleanup, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_NOLOCK, store->pool);

		ks_log(KS_LOG_INFO, "Removing identity %s from node %s", key, val);

		ks_hash_insert(cleanup, key, (void *)KS_TRUE);
	}

    if (cleanup) {
		for (itt = ks_hash_first(cleanup, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
			const char *key = NULL;
			void *val = NULL;

			ks_hash_this(itt, (const void **)&key, NULL, (void **)&val);

			ks_hash_remove(store->identities, (const void *)key);
		}
		ks_hash_destroy(&cleanup);
	}
	
	ks_hash_write_unlock(store->identities);
}

static ks_status_t __get_node_identities(swclt_store_t *store,
										 const char *nodeid,
										 ks_pool_t *pool,
										 ks_hash_t **identities)
{
	ks_status_t status = KS_STATUS_SUCCESS;
    ks_hash_iterator_t *itt;

	ks_hash_create(identities, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_FREE_KEY, pool);

    ks_hash_read_lock(store->identities);
    // iterate all identities
    for (itt = ks_hash_first(store->identities, KS_UNLOCKED); itt; itt = ks_hash_next(&itt)) {
		const char *val;
		const char *key;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&val);

		if (strcmp(nodeid, val)) continue;

		ks_hash_insert(*identities, ks_pstrdup(pool, key), (void *)KS_TRUE);
	}

    ks_hash_read_unlock(store->identities);
}

static void __remove_provider_from_protocols(swclt_store_t *store, const char *nodeid)
{
	ks_hash_iterator_t *itt;
	ks_hash_t *cleanup = NULL;
	ks_json_t *entry = NULL;

	ks_log(KS_LOG_INFO, "Request to remove provider %s", nodeid);

	ks_hash_write_lock(store->protocols);
	// iterate all protocols
	for (itt = ks_hash_first(store->protocols, KS_UNLOCKED); itt; ) {
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
			ks_assertd(!BLADE_PROVIDER_PARSE(store->pool, entry, &provider));

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
						 ks_hash_create(&cleanup, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_NOLOCK, store->pool);

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
			char *protocol_name = ks_pstrdup(store->pool, protocol->name);
			ks_hash_remove(store->protocols, (const void *)key); // now destroy it...
			__invoke_cb_protocol_remove(store, protocol_name);
			ks_pool_free(&protocol_name);
		}
		ks_hash_destroy(&cleanup);
	}

	ks_hash_write_unlock(store->protocols);
}

static ks_status_t __select_random_protocol_provider(swclt_store_t *store, const char *name, ks_pool_t *pool, char **providerid)
{
	blade_protocol_t *protocol = NULL;
	int32_t count = 0;
	int32_t index = 0;
	ks_json_t *entry = NULL;
	blade_provider_t *provider = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;

	*providerid = NULL;

	ks_hash_write_lock(store->protocols);

	// find the protocol
	if (status = __lookup_protocol(store, name, &protocol))
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
	ks_assertd(!BLADE_PROVIDER_PARSE(store->pool, entry, &provider));

	// copy provider id
	*providerid = ks_pstrdup(pool, provider->nodeid);

	// cleanup provider
	BLADE_PROVIDER_DESTROY(&provider);

done:
	ks_hash_write_unlock(store->protocols);

	return status;
}

static ks_status_t __get_protocols(swclt_store_t *store,
								   ks_json_t **protocols)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	*protocols = ks_json_create_array();

	ks_hash_read_lock(store->protocols);

	for (ks_hash_iterator_t *it = ks_hash_first(store->protocols, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
		const char *key = NULL;
		blade_protocol_t *proto = NULL;

		ks_hash_this(it, (const void **)&key, NULL, (void **)&proto);

		ks_json_add_string_to_array(*protocols, key);
	}

	ks_hash_read_unlock(store->protocols);
	return status;
}

static ks_status_t __get_protocol_providers(swclt_store_t *store,
											const char *name,
											ks_json_t **providers)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	blade_protocol_t *protocol = NULL;

	*providers = NULL;

	ks_hash_read_lock(store->protocols);

	if (status = __lookup_protocol(store, name, &protocol))
		goto done;

	if (ks_json_get_array_size(protocol->providers) == 0) {
		status = KS_STATUS_NOT_FOUND;
		goto done;
	}

	*providers = ks_json_duplicate(protocol->providers, KS_TRUE);

done:
	ks_hash_read_unlock(store->protocols);
	return status;
}

static ks_status_t __add_authority_obj(swclt_store_t *store, ks_json_t *obj)
{
	blade_netcast_authority_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_AUTHORITY_ADD_PARAM_PARSE(store->pool, obj, &params)) {
		ks_log(KS_LOG_ERROR, "Failed to parse authority: %d", status);
		return status;
	}

	if (status = ks_hash_insert(store->authorities, ks_pstrdup(store->pool, params->nodeid), (void *)KS_TRUE)) {
		ks_log(KS_LOG_ERROR, "Failed to insert authority: %d", status);
		BLADE_NETCAST_AUTHORITY_ADD_PARAM_DESTROY(&params);
		return status;
	}
	
	BLADE_NETCAST_AUTHORITY_ADD_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Protocol add/remove for uncertified clients only
static ks_status_t __update_protocol_add(swclt_store_t *store, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_add_param_t *params = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;

	if (status = BLADE_NETCAST_PROTOCOL_ADD_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	ks_log(KS_LOG_INFO, "Request to add new protocol %s", params->protocol);

	/* Lookup the protocol */
	if (status = __lookup_protocol_uncertified(store, params->protocol)) {
		char *key = ks_pstrdup(ks_pool_get(store->protocols_uncertified), params->protocol);
		
		ks_log(KS_LOG_INFO, "Protocol %s does not exist yet, adding new entry", params->protocol);

		/* And add it */
		if (status = ks_hash_insert(store->protocols_uncertified, key, (void *)KS_TRUE)) {
			ks_pool_free(&key);
			BLADE_NETCAST_PROTOCOL_ADD_PARAM_DESTROY(&params);
			return status;
		}

		ks_log(KS_LOG_INFO, "Protocol %s added", params->protocol);

		__invoke_cb_protocol_add(store, params->protocol);

		return status;

	}

	ks_log(KS_LOG_INFO, "Protocol %s exists already", params->protocol);

	BLADE_NETCAST_PROTOCOL_ADD_PARAM_DESTROY(&params);
	
	return status;
}

static ks_status_t __update_protocol_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_remove_param_t *params = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t match = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_REMOVE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	match = ks_hash_remove(store->protocols_uncertified, (const void *)params->protocol) != NULL;

done:

	if (match) __invoke_cb_protocol_remove(store, params->protocol);

	BLADE_NETCAST_PROTOCOL_REMOVE_PARAM_DESTROY(&params);

	return status;
}

// Provider add/remove
static ks_status_t __update_protocol_provider_add(swclt_store_t *store, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_add_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_json_t *provider_data = NULL;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	ks_log(KS_LOG_INFO, "Request to add new provider %s for protocol %s", params->nodeid, params->protocol);

	ks_hash_write_lock(store->protocols);

	/* Lookup the protocol */
	if (status = __lookup_protocol(store, params->protocol, &protocol)) {
		ks_log(KS_LOG_INFO, "Protocol %s does not exist yet, adding new entry", params->protocol);

		/* Gotta add a new one */
		if (!(protocol = ks_pool_alloc(store->pool, sizeof(blade_protocol_t)))) {
			ks_hash_write_unlock(store->protocols);
			BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);
			return KS_STATUS_NO_MEM;
		}

		protocol->channels = ks_json_duplicate(params->channels, KS_TRUE);

		protocol->default_channel_broadcast_access = params->default_channel_broadcast_access;
		protocol->default_channel_subscribe_access = params->default_channel_subscribe_access;
		protocol->default_method_execute_access = params->default_method_execute_access;

		protocol->name = ks_pstrdup(store->pool, params->protocol);

		if (params->data) {
			provider_data = ks_json_duplicate(params->data, KS_TRUE);
		}

		if (!(protocol->providers = ks_json_create_array())) {
			ks_json_delete(&protocol->channels);
			ks_pool_free(&protocol);
			ks_json_delete(&provider_data);
			BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);
			ks_hash_write_unlock(store->protocols);
			return KS_STATUS_NO_MEM;
		}
		ks_json_add_item_to_array(protocol->providers,
			BLADE_PROVIDER_MARSHAL(
				&(blade_provider_t){params->nodeid, NULL, params->rank, provider_data}
			)
		);

		/* And add it */
		if (status = ks_hash_insert(store->protocols, protocol->name, protocol)) {
			ks_json_delete(&protocol->channels);
			ks_json_delete(&protocol->providers);
			ks_pool_free(&protocol);
			BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);
			ks_hash_write_unlock(store->protocols);
			return status;

		}

		ks_log(KS_LOG_INFO, "Protocol %s added with provider %s", protocol->name, params->nodeid);

		// @todo come back and fix this, params->protocol is going to be NULL, need to duplicate
		// the string so it remains valid in params
		__invoke_cb_protocol_add(store, params->protocol);
		__invoke_cb_protocol_provider_add(store, netcast_rqu, params);

		ks_hash_write_unlock(store->protocols);

		BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);

		return status;

	}

	ks_log(KS_LOG_INFO, "Protocol %s exists already, has %lu providers currently",
				params->protocol, ks_json_get_array_size(protocol->providers));

	/* Now add any provider entries to the protocol */
	if (params->data) {
		provider_data = ks_json_duplicate(params->data, KS_TRUE);
	}
	ks_json_add_item_to_array(protocol->providers,
					BLADE_PROVIDER_MARSHAL(&(blade_provider_t){params->nodeid, NULL, params->rank, provider_data}));

	ks_log(KS_LOG_INFO, "Protocol %s add complete, provider count %lu", protocol->name, ks_json_get_array_size(protocol->providers));

	__invoke_cb_protocol_provider_add(store, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_DESTROY(&params);

	ks_hash_write_unlock(store->protocols);
	
	return status;
}

static ks_status_t __update_protocol_provider_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_remove_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_json_t *entry = NULL;
	int32_t index = 0;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t match = KS_FALSE;
	ks_bool_t cleanup = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_REMOVE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(store->protocols);

	if (status = __lookup_protocol(store, params->protocol, &protocol)) {
		ks_log(KS_LOG_WARNING, "Received protocol provider remove for protocol '%s' which does not exist", params->protocol);
		status = KS_STATUS_SUCCESS;
		goto done;
	}

		// iterate protocol providers
	KS_JSON_ARRAY_FOREACH(entry, protocol->providers) {
		blade_provider_t *provider;

		// parse provider
		ks_assertd(!BLADE_PROVIDER_PARSE(store->pool, entry, &provider));

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
		char *protocol_name = ks_pstrdup(store->pool, protocol->name);
		ks_hash_remove(store->protocols, (const void *)protocol->name); // now destroy it...
		__invoke_cb_protocol_remove(store, protocol_name);
		ks_pool_free(&protocol_name);
	}

done:
	ks_hash_write_unlock(store->protocols);

	if (match) __invoke_cb_protocol_provider_remove(store, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_REMOVE_PARAM_DESTROY(&params);

	return status;
}

// Provider rank update
static ks_status_t __update_protocol_provider_rank_update(swclt_store_t *store, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_rank_update_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_json_t *entry = NULL;
	blade_provider_t *provider = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t found = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(store->protocols);

	ks_log(KS_LOG_INFO, "Request to update rank for provider %s to %d for protocol %s", params->nodeid, params->rank, params->protocol);

	/* Lookup the protocol */
	if (status = __lookup_protocol(store, params->protocol, &protocol))
		goto done;

	// find provider
	for (int32_t index = 0; index < ks_json_get_array_size(protocol->providers); ++index) {
		entry = ks_json_get_array_item(protocol->providers, index);

		const char *provider_nodeid = ks_json_get_object_string(entry, "nodeid", "");
		if (!strcmp(provider_nodeid, params->nodeid)) {
			found = KS_TRUE;
			ks_json_delete_item_from_object(entry, "rank");
			ks_json_add_number_to_object(entry, "rank", params->rank);
		}
	}

done:
	ks_hash_write_unlock(store->protocols);

	if (found) __invoke_cb_protocol_provider_rank_update(store, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM_DESTROY(&params);
	
	return status;
}

// Provider data update
static ks_status_t __update_protocol_provider_data_update(swclt_store_t *store, const blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_protocol_provider_data_update_param_t *params = NULL;
	blade_protocol_t *protocol = NULL;
	ks_json_t *entry = NULL;
	blade_provider_t *provider = NULL;
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_bool_t found = KS_FALSE;

	if (status = BLADE_NETCAST_PROTOCOL_PROVIDER_DATA_UPDATE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(store->protocols);

	ks_log(KS_LOG_INFO, "Request to update data for provider %s for protocol %s", params->nodeid, params->protocol);

	/* Lookup the protocol */
	if (status = __lookup_protocol(store, params->protocol, &protocol))
		goto done;

	// find provider
	for (int32_t index = 0; index < ks_json_get_array_size(protocol->providers); ++index) {
		entry = ks_json_get_array_item(protocol->providers, index);
		const char *provider_nodeid = ks_json_get_object_string(entry, "nodeid", "");
		if (!strcmp(provider_nodeid, params->nodeid)) {
			found = KS_TRUE;
			ks_json_delete_item_from_object(entry, "data");
			ks_json_add_item_to_object(entry, "data", ks_json_duplicate(params->data, KS_TRUE));
		}
	}

done:
	ks_hash_write_unlock(store->protocols);

	if (found) __invoke_cb_protocol_provider_data_update(store, netcast_rqu, params);

	BLADE_NETCAST_PROTOCOL_PROVIDER_DATA_UPDATE_PARAM_DESTROY(&params);
	
	return status;
}


// Route add/remove
static ks_status_t __update_route_add(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_route_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_ROUTE_ADD_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	status = __add_route_obj(store, netcast_rqu->params);

	__invoke_cb_route_add(store, netcast_rqu, params);

	BLADE_NETCAST_ROUTE_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_route_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_route_remove_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_ROUTE_REMOVE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	__invoke_cb_route_remove(store, netcast_rqu, params);

	/* Remove the node from the hash */
	ks_hash_remove(store->routes, params->nodeid);

	/* Remove the identities that map to the nodeid */
	__remove_identities_by_nodeid(store, params->nodeid);

	/* Now we have to rummage through the protocols hash and remove any protocols hosted by this node */
	__remove_provider_from_protocols(store, params->nodeid);

	// @todo purge subscribers with the given nodeid
	
	ks_hash_remove(store->authorities, params->nodeid);
	//ks_hash_remove(store->accesses, params->nodeid);

	/* Done with the request */
	BLADE_NETCAST_ROUTE_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Identity add/remove
static ks_status_t __update_identity_add(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_identity_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_IDENTITY_ADD_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

    if (status = ks_hash_insert(store->identities, ks_pstrdup(store->pool, params->identity), ks_pstrdup(store->pool, params->nodeid))) {
		ks_log(KS_LOG_ERROR, "Failed to insert identity: %d", status);
		goto done;
	}
	
	__invoke_cb_identity_add(store, netcast_rqu, params);

done:
	BLADE_NETCAST_IDENTITY_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_identity_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_identity_remove_param_t *params;
	ks_status_t status;
	const char *nodeid = NULL;

	if (status = BLADE_NETCAST_IDENTITY_REMOVE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	ks_hash_write_lock(store->identities);

	/* Make sure the identity is owned by the right nodeid */
	nodeid = (const char *)ks_hash_search(store->identities, params->identity, KS_UNLOCKED);
	if (nodeid && !strcmp(nodeid, params->nodeid))
	{
		/* Remove the identity from the hash */
		__invoke_cb_identity_remove(store, netcast_rqu, params);
		ks_hash_remove(store->identities, params->identity);
	}

	ks_hash_write_unlock(store->identities);

	/* Done with the request */
	BLADE_NETCAST_IDENTITY_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Channel add/remove
static ks_status_t __update_channel_add(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update_channel_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

// Subscription add/remove
static ks_status_t __update_subscription_add(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_subscription_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_SUBSCRIPTION_ADD_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	/* @TODO add subscription to store->subscriptions */

	__invoke_cb_subscription_add(store, netcast_rqu, params);

	BLADE_NETCAST_SUBSCRIPTION_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_subscription_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_subscription_remove_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_SUBSCRIPTION_REMOVE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	/* @TODO Remove the subscription from store->subscriptions */

	__invoke_cb_subscription_remove(store, netcast_rqu, params);

	/* Done with the request */
	BLADE_NETCAST_SUBSCRIPTION_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Authorization add/remove
static ks_status_t __update_authorization_add(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update_authorization_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

// Authority add/remove
static ks_status_t __update_authority_add(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_authority_add_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_AUTHORITY_ADD_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	status = __add_authority_obj(store, netcast_rqu->params);

	__invoke_cb_authority_add(store, netcast_rqu, params);

	BLADE_NETCAST_AUTHORITY_ADD_PARAM_DESTROY(&params);

	return status;
}

static ks_status_t __update_authority_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	blade_netcast_authority_remove_param_t *params;
	ks_status_t status;

	if (status = BLADE_NETCAST_AUTHORITY_REMOVE_PARAM_PARSE(store->pool, netcast_rqu->params, &params))
		return status;

	/* Remove the node from the hash */
	ks_hash_remove(store->authorities, params->nodeid);

	__invoke_cb_authority_remove(store, netcast_rqu, params);

	/* Done with the request */
	BLADE_NETCAST_AUTHORITY_REMOVE_PARAM_DESTROY(&params);

	return KS_STATUS_SUCCESS;
}

// Access add/remove
static ks_status_t __update_access_add(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update_access_remove(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	/* @@ TODO */
	return KS_STATUS_SUCCESS;
}

static ks_status_t __update(swclt_store_t *store, blade_netcast_rqu_t *netcast_rqu)
{
	/* Based on the command, do the appropriate update call */
	if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_ADD))
		return __update_protocol_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_REMOVE))
		return __update_protocol_remove(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD))
		return __update_protocol_provider_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE))
		return __update_protocol_provider_remove(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE))
		return __update_protocol_provider_rank_update(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE))
		return __update_protocol_provider_data_update(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ROUTE_ADD))
		return __update_route_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ROUTE_REMOVE))
		return __update_route_remove(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_IDENTITY_ADD))
		return __update_identity_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_IDENTITY_REMOVE))
		return __update_identity_remove(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_SUBSCRIPTION_ADD))
		return __update_subscription_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE))
		return __update_subscription_remove(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORITY_ADD))
		return __update_authority_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORITY_REMOVE))
		return __update_authority_remove(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORIZATION_ADD))
		return __update_authorization_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_AUTHORIZATION_REMOVE))
		return __update_authorization_remove(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ACCESS_ADD))
		return __update_access_add(store, netcast_rqu);
	else if (!strcmp(netcast_rqu->command, BLADE_NETCAST_CMD_ACCESS_REMOVE))
		return __update_access_remove(store, netcast_rqu);
	else {
		ks_log(KS_LOG_WARNING, "Unknown netcast subcommand: %s", netcast_rqu->command);
		return KS_STATUS_SUCCESS;
	}
}

static ks_status_t __populate_routes(swclt_store_t *store, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;
	const char *nodeid;
	ks_json_t *identities;

	/* Walk the routes array and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->routes) {
		if (status = __add_route_obj(store, entry)) {
			ks_log(KS_LOG_ERROR, "Failed to populate route: %d", status);
			return status;
		}

		// Identities
		nodeid = ks_json_get_object_string(entry, "nodeid", NULL);
		identities = ks_json_get_object_item(entry, "identities");
		if (nodeid && identities && ks_json_type_is_array(identities))
		{
			int size = ks_json_get_array_size(identities);
			for (int index = 0; index < size; ++index)
			{
				const char *identity = ks_json_get_array_string(identities, index, NULL);
				if (!identity) continue;
				ks_hash_insert(store->identities, ks_pstrdup(store->pool, identity), ks_pstrdup(store->pool, nodeid));
			}
		}
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __populate_protocols(swclt_store_t *store, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->protocols) {
		if (status = __add_protocol_obj(store, entry)) {
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

static ks_status_t __populate_subscriptions(swclt_store_t *store, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->subscriptions) {
		blade_subscription_t *subscription;

		if (status = BLADE_SUBSCRIPTION_PARSE(store->pool, entry, &subscription)) {
			ks_log(KS_LOG_ERROR, "Failed to parse subscription: %d", status);
			return status;
		}

		/* Subscriptions get keyed by a combo key with the channel/protocol */
		if (status = ks_hash_insert(store->subscriptions, ks_psprintf(store->pool, "%s:%s", subscription->protocol, subscription->channel), subscription)) {
			ks_log(KS_LOG_ERROR, "Failed to insert subscription: %d", status);
			BLADE_SUBSCRIPTION_DESTROY(&subscription);
			return status;
		}
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __populate_authorities(swclt_store_t *store, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->authorities) {
		const char *authority;
		if (!ks_json_value_string(entry, &authority)) {
			authority = ks_pstrdup(ks_pool_get(store->authorities), authority);

			if (status = ks_hash_insert(store->authorities, authority, (void *)KS_TRUE)) {
				ks_log(KS_LOG_ERROR, "Failed to insert authority: %d", status);
				ks_pool_free(&authority);
				return status;
			}
		}
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __populate_protocols_uncertified(swclt_store_t *store, blade_connect_rpl_t *connect_rpl)
{
	ks_json_t *entry;
	ks_status_t status;

	/* Walk the protocols and add them */
	KS_JSON_ARRAY_FOREACH(entry, connect_rpl->protocols_uncertified) {
		if (status = __add_protocol_uncertified_obj(store, entry)) {
			ks_log(KS_LOG_ERROR, "Failed to populate protocol for uncertified client: %d", status);
			return status;
		}
	}

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(char *) swclt_store_describe(swclt_store_t *store)
{
	return ks_psprintf(store->pool, "SWCLT Store: protocols %d routes %d authorities: %d subscriptions: %d",
		ks_hash_count(store->protocols), ks_hash_count(store->routes), ks_hash_count(store->authorities), ks_hash_count(store->subscriptions));
}

SWCLT_DECLARE(ks_status_t) swclt_store_destroy(swclt_store_t **storeP)
{
	if (storeP && *storeP) {
		swclt_store_t *store = *storeP;
		ks_pool_t *pool = store->pool;
		*storeP = NULL;
		ks_hash_destroy(&store->callbacks);
		ks_hash_destroy(&store->subscriptions);
		ks_hash_destroy(&store->protocols);
		ks_hash_destroy(&store->identities);
		ks_hash_destroy(&store->routes);
		ks_hash_destroy(&store->authorities);
		ks_hash_destroy(&store->protocols_uncertified);
		ks_pool_close(&pool);
	}
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_store_create(swclt_store_t **storeP)
{
	ks_status_t status;
	swclt_store_t *store = NULL;
	ks_pool_t *pool = NULL;

	ks_pool_open(&pool);
	store = ks_pool_alloc(pool, sizeof(swclt_store_t));
	store->pool = pool;
	*storeP = store;

	if (status = ks_hash_create(
			&store->callbacks,
			KS_HASH_MODE_INT,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_RWLOCK,
			store->pool))
		goto done;

	/* Create our authorities hash, keyed by the node id of the authority */
	if (status = ks_hash_create(
			&store->authorities,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			store->pool))
		goto done;

	/* Create our subscriptions hash, keyed by the protocol + channel name */
	if (status = ks_hash_create(
			&store->subscriptions,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			store->pool))
		goto done;
	ks_hash_set_destructor(store->subscriptions, __destroy_subscription);

	/* Create our protocols hash, keyed by the protocol name */
	if (status = ks_hash_create(
			&store->protocols,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK,  /* the key is inside the value struct - only need to destroy value */
			store->pool))
		goto done;
	ks_hash_set_destructor(store->protocols, __destroy_protocol);

	/* Create our routes hash, keyed by the nodeid */
	if (status = ks_hash_create(
			&store->routes,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			store->pool))
		goto done;
	ks_hash_set_destructor(store->routes, __destroy_node);

	/* Create our identities hash, keyed by the identity */
	if (status = ks_hash_create(
			&store->identities,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY | KS_HASH_FLAG_FREE_VALUE,
			store->pool))
		goto done;

	/* Create our uncertified client protocols hash, keyed by the protocol name */
	if (status = ks_hash_create(
			&store->protocols_uncertified,
			KS_HASH_MODE_CASE_INSENSITIVE,
			KS_HASH_FLAG_RWLOCK | KS_HASH_FLAG_DUP_CHECK | KS_HASH_FLAG_FREE_KEY,
			store->pool))
		goto done;

done:
	if (status != KS_STATUS_SUCCESS) {
		swclt_store_destroy(storeP);
	}

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_store_reset(swclt_store_t *store)
{
	ks_hash_iterator_t *itt;
	ks_status_t status = KS_STATUS_SUCCESS;

	ks_hash_write_lock(store->routes);
	for (itt = ks_hash_first(store->routes, KS_UNLOCKED); itt; ) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		itt = ks_hash_next(&itt);
		ks_hash_remove(store->routes, (const void *)key);
	}
	ks_hash_write_unlock(store->routes);

	ks_hash_write_lock(store->identities);
	for (itt = ks_hash_first(store->identities, KS_UNLOCKED); itt; ) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		itt = ks_hash_next(&itt);
		ks_hash_remove(store->identities, (const void *)key);
	}
	ks_hash_write_unlock(store->identities);

	ks_hash_write_lock(store->protocols);
	for (itt = ks_hash_first(store->protocols, KS_UNLOCKED); itt; ) {
		const char *key = NULL;
		blade_protocol_t *protocol = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&protocol);

		itt = ks_hash_next(&itt);
		ks_hash_remove(store->protocols, (const void *)key);
	}
	ks_hash_write_unlock(store->protocols);

	ks_hash_write_lock(store->subscriptions);
	for (itt = ks_hash_first(store->subscriptions, KS_UNLOCKED); itt; ) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		itt = ks_hash_next(&itt);
		ks_hash_remove(store->subscriptions, (const void *)key);
	}
	ks_hash_write_unlock(store->subscriptions);

	ks_hash_write_lock(store->authorities);
	for (itt = ks_hash_first(store->authorities, KS_UNLOCKED); itt; ) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		itt = ks_hash_next(&itt);
		ks_hash_remove(store->authorities, (const void *)key);
	}
	ks_hash_write_unlock(store->authorities);

	ks_hash_write_lock(store->protocols_uncertified);
	for (itt = ks_hash_first(store->protocols_uncertified, KS_UNLOCKED); itt; ) {
		const char *key = NULL;
		void *value = NULL;

		ks_hash_this(itt, (const void **)&key, NULL, (void **)&value);

		itt = ks_hash_next(&itt);
		ks_hash_remove(store->protocols_uncertified, (const void *)key);
	}
	ks_hash_write_unlock(store->protocols_uncertified);

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_store_populate(swclt_store_t *store, const blade_connect_rpl_t *connect_rpl)
{
	blade_connect_rpl_t *rpl = (blade_connect_rpl_t *)connect_rpl;
	ks_status_t status = KS_STATUS_SUCCESS;

	/* Now popualte the fields from the connect result */
	if (status = __populate_routes(store, rpl))
		return status;

	if (status = __populate_protocols(store, rpl))
		return status;

	if (status = __populate_subscriptions(store, rpl))
		return status;

	if (status = __populate_authorities(store, rpl))
		return status;

	if (status = __populate_protocols_uncertified(store, rpl))
		return status;

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_store_get_node_identities(swclt_store_t *store,
														   const char *nodeid,
														   ks_pool_t *pool,
														   ks_hash_t **identities)
{
	return __get_node_identities(store, nodeid, pool, identities);
}

SWCLT_DECLARE(ks_status_t) swclt_store_get_protocols(swclt_store_t *store, ks_json_t **protocols)
{
	return __get_protocols(store, protocols);
}

/* Returns success if the protocol exists and the store is valid, also works for uncertified client protocols check. */
SWCLT_DECLARE(ks_status_t) swclt_store_check_protocol(swclt_store_t *store, const char *name)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	if (status = __lookup_protocol(store, name, NULL)) {
		status = __lookup_protocol_uncertified(store, name);
	}
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_store_select_random_protocol_provider(
	swclt_store_t *store,
   	const char *name,
	ks_pool_t *pool,
	char **providerid)
{
	return __select_random_protocol_provider(store, name, pool, providerid);
}

SWCLT_DECLARE(ks_status_t) swclt_store_get_protocol_providers(swclt_store_t *store,
															  const char *name,
															  ks_json_t **providers)
{
	return __get_protocol_providers(store, name, providers);
}

SWCLT_DECLARE(ks_status_t) swclt_store_update(swclt_store_t *store, const blade_netcast_rqu_t *netcast_rqu)
{
	return __update(store, (blade_netcast_rqu_t *) netcast_rqu);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_route_add(swclt_store_t *store, swclt_store_cb_route_add_t cb)
{
	return __add_cb_route_add(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_route_remove(swclt_store_t *store, swclt_store_cb_route_remove_t cb)
{
	return __add_cb_route_remove(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_identity_add(swclt_store_t *store, swclt_store_cb_identity_add_t cb)
{
	return __add_cb_identity_add(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_identity_remove(swclt_store_t *store, swclt_store_cb_identity_remove_t cb)
{
	return __add_cb_identity_remove(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_add(swclt_store_t *store, swclt_store_cb_protocol_add_t cb)
{
	return __add_cb_protocol_add(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_remove(swclt_store_t *store, swclt_store_cb_protocol_remove_t cb)
{
	return __add_cb_protocol_remove(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_add(swclt_store_t *store, swclt_store_cb_protocol_provider_add_t cb)
{
	return __add_cb_protocol_provider_add(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_remove(swclt_store_t *store, swclt_store_cb_protocol_provider_remove_t cb)
{
	return __add_cb_protocol_provider_remove(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_rank_update(swclt_store_t *store, swclt_store_cb_protocol_provider_rank_update_t cb)
{
	return __add_cb_protocol_provider_rank_update(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_protocol_provider_data_update(swclt_store_t *store, swclt_store_cb_protocol_provider_data_update_t cb)
{
	return __add_cb_protocol_provider_data_update(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_authority_add(swclt_store_t *store, swclt_store_cb_authority_add_t cb)
{
	return __add_cb_authority_add(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_authority_remove(swclt_store_t *store, swclt_store_cb_authority_remove_t cb)
{
	return __add_cb_authority_remove(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_subscription_add(swclt_store_t *store, swclt_store_cb_subscription_add_t cb)
{
	return __add_cb_subscription_add(store, cb);
}

SWCLT_DECLARE(ks_status_t) swclt_store_cb_subscription_remove(swclt_store_t *store, swclt_store_cb_subscription_remove_t cb)
{
	return __add_cb_subscription_remove(store, cb);
}
