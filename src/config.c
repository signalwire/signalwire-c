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
#include "signalwire-client-c/internal/config.h"

SWCLT_DECLARE(ks_status_t) swclt_config_create(swclt_config_t **config)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	ks_pool_t *pool = NULL;

	ks_assert(config);
	
	ks_pool_open(&pool);

	*config = ks_pool_alloc(pool, sizeof(swclt_config_t));

	return ret;
}

SWCLT_DECLARE(ks_status_t) swclt_config_destroy(swclt_config_t **config)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	ks_pool_t *pool = NULL;
	
	ks_assert(config);
	ks_assert(*config);

	if ((*config)->network) ks_json_delete(&(*config)->network);

	pool = ks_pool_get(*config);
	ks_pool_close(&pool);

	*config = NULL;

	return ret;
}

SWCLT_DECLARE(ks_status_t) swclt_config_set_default_network(swclt_config_t *config, ks_bool_t allData)
{
	if (config->network) ks_json_delete(&config->network);
	config->network = ks_json_create_object();
	ks_json_add_bool_to_object(config->network, "route_data", allData);
	ks_json_add_bool_to_object(config->network, "route_add", allData);
	ks_json_add_bool_to_object(config->network, "route_remove", allData);
	ks_json_add_bool_to_object(config->network, "authority_data", allData);
	ks_json_add_bool_to_object(config->network, "authority_add", allData);
	ks_json_add_bool_to_object(config->network, "authority_remove", allData);
	ks_json_add_bool_to_object(config->network, "filtered_protocols", !allData);
	ks_json_add_array_to_object(config->network, "protocols");
}

SWCLT_DECLARE(ks_status_t) swclt_config_load_from_json(swclt_config_t *config, ks_json_t *json)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	const char *val = NULL;
	ks_json_t *network = NULL;

	if ((val = ks_json_get_object_string(json, "private_key_path", NULL))) {
		swclt_config_set_private_key_path(config, val);
	}

	if ((val = ks_json_get_object_string(json, "client_cert_path", NULL))) {
		swclt_config_set_client_cert_path(config, val);
	}

	if ((val = ks_json_get_object_string(json, "cert_chain_path", NULL))) {
		swclt_config_set_cert_chain_path(config, val);
	}
	
	if ((val = ks_json_get_object_string(json, "authentication", NULL))) {
		swclt_config_set_authentication(config, val);
	}

	if ((val = ks_json_get_object_string(json, "agent", NULL))) {
		swclt_config_set_agent(config, val);
	}

	if ((val = ks_json_get_object_string(json, "identity", NULL))) {
		swclt_config_set_identity(config, val);
	}
	
	if ((network = ks_json_get_object_item(json, "network"))) {
		if (config->network) ks_json_delete(&config->network);
		config->network = ks_json_duplicate(network, KS_TRUE);
		if (ks_json_get_object_item(config->network, "protocols") == NULL) ks_json_add_array_to_object(config->network, "protocols");
	} else {
		swclt_config_set_default_network(config, KS_TRUE);
	}
	
	return ret;
}

SWCLT_DECLARE(ks_status_t) swclt_config_load_from_env(swclt_config_t *config)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	const char *val = NULL;
	char protocolKey[256];
	int protocolCount = 0;

	if ((val = getenv("SW_PRIVATE_KEY_PATH"))) {
		swclt_config_set_private_key_path(config, val);
	}
	
	if ((val = getenv("SW_CLIENT_CERT_PATH"))) {
		swclt_config_set_client_cert_path(config, val);
	}
	
	if ((val = getenv("SW_CERT_CHAIN_PATH"))) {
		swclt_config_set_cert_chain_path(config, val);
	}
	
	if ((val = getenv("SW_AUTHENTICATION"))) {
		swclt_config_set_authentication(config, val);
	}

	if ((val = getenv("SW_AGENT"))) {
		swclt_config_set_agent(config, val);
	}

	if ((val = getenv("SW_IDENTITY"))) {
		swclt_config_set_identity(config, val);
	}

	if ((val = getenv("SW_NETWORK_ROUTE_DATA"))) {
		swclt_config_set_network_route_data(config, val[0] == '1' || val[0] == 't' || val[0] == 'T');
	}

	if ((val = getenv("SW_NETWORK_ROUTE_ADD"))) {
		swclt_config_set_network_route_add(config, val[0] == '1' || val[0] == 't' || val[0] == 'T');
	}

	if ((val = getenv("SW_NETWORK_ROUTE_REMOVE"))) {
		swclt_config_set_network_route_remove(config, val[0] == '1' || val[0] == 't' || val[0] == 'T');
	}

	if ((val = getenv("SW_NETWORK_AUTHORITY_DATA"))) {
		swclt_config_set_network_authority_data(config, val[0] == '1' || val[0] == 't' || val[0] == 'T');
	}

	if ((val = getenv("SW_NETWORK_AUTHORITY_ADD"))) {
		swclt_config_set_network_authority_add(config, val[0] == '1' || val[0] == 't' || val[0] == 'T');
	}

	if ((val = getenv("SW_NETWORK_AUTHORITY_REMOVE"))) {
		swclt_config_set_network_authority_remove(config, val[0] == '1' || val[0] == 't' || val[0] == 'T');
	}

	if ((val = getenv("SW_NETWORK_FILTERED_PROTOCOLS"))) {
		swclt_config_set_network_filtered_protocols(config, val[0] == '1' || val[0] == 't' || val[0] == 'T');
	}

	if (!config->network) {
		swclt_config_set_default_network(config, KS_TRUE);
	} else {
		ks_json_t *protocols = NULL;
		while (1) {
			ks_snprintf(protocolKey, sizeof(protocolKey), "SW_NETWORK_PROTOCOL_%d", protocolCount);
			++protocolCount;
			if (!(val = getenv(protocolKey)) || !val[0]) break;
			if (!protocols) {
				protocols = ks_json_get_object_item(config->network, "protocols");
				if (!protocols) protocols = ks_json_add_array_to_object(config->network, "protocols");
			}
			ks_json_add_string_to_array(protocols, val);
		}
	}

	
	return ret;
}

													  
SWCLT_DECLARE(const char *) swclt_config_get_private_key_path(swclt_config_t *config)
{
	ks_assert(config);

	return config->private_key_path;
}

SWCLT_DECLARE(ks_status_t) swclt_config_set_private_key_path(swclt_config_t *config, const char *value)
{
	ks_assert(config);

	if (config->private_key_path) ks_pool_free(&config->private_key_path);
	if (value) config->private_key_path = ks_pstrdup(ks_pool_get(config), value);

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(const char *) swclt_config_get_client_cert_path(swclt_config_t *config)
{
	ks_assert(config);

	return config->client_cert_path;
}

SWCLT_DECLARE(ks_status_t) swclt_config_set_client_cert_path(swclt_config_t *config, const char *value)
{
	ks_assert(config);

	if (config->client_cert_path) ks_pool_free(&config->client_cert_path);
	if (value) config->client_cert_path = ks_pstrdup(ks_pool_get(config), value);

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(const char *) swclt_config_get_cert_chain_path(swclt_config_t *config)
{
	ks_assert(config);

	return config->cert_chain_path;
}

SWCLT_DECLARE(ks_status_t) swclt_config_set_cert_chain_path(swclt_config_t *config, const char *value)
{
	ks_assert(config);

	if (config->cert_chain_path) ks_pool_free(&config->cert_chain_path);
	if (value) config->cert_chain_path = ks_pstrdup(ks_pool_get(config), value);

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(const char *) swclt_config_get_authentication(swclt_config_t *config)
{
	ks_assert(config);

	return config->authentication;
}

SWCLT_DECLARE(ks_status_t) swclt_config_set_authentication(swclt_config_t *config, const char *value)
{
	ks_assert(config);

	if (config->authentication) ks_pool_free(&config->authentication);
	if (value) config->authentication = ks_pstrdup(ks_pool_get(config), value);

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(const char *) swclt_config_get_agent(swclt_config_t *config)
{
	ks_assert(config);

	return config->agent;
}

SWCLT_DECLARE(ks_status_t) swclt_config_set_agent(swclt_config_t *config, const char *value)
{
	ks_assert(config);

	if (config->agent) ks_pool_free(&config->agent);
	if (value) config->agent = ks_pstrdup(ks_pool_get(config), value);

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(const char *) swclt_config_get_identity(swclt_config_t *config)
{
	ks_assert(config);

	return config->identity;
}

SWCLT_DECLARE(ks_status_t) swclt_config_set_identity(swclt_config_t *config, const char *value)
{
	ks_assert(config);

	if (config->identity) ks_pool_free(&config->identity);
	if (value) config->identity = ks_pstrdup(ks_pool_get(config), value);

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_route_data(swclt_config_t *config)
{
	ks_assert(config);

	return ks_json_get_object_bool(config->network, "route_data", KS_FALSE);
}

SWCLT_DECLARE(void) swclt_config_set_network_route_data(swclt_config_t *config, ks_bool_t value)
{
	ks_assert(config);

	ks_json_add_bool_to_object(config->network, "route_data", value);
}

SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_route_add(swclt_config_t *config)
{
	ks_assert(config);

	return ks_json_get_object_bool(config->network, "route_add", KS_FALSE);
}

SWCLT_DECLARE(void) swclt_config_set_network_route_add(swclt_config_t *config, ks_bool_t value)
{
	ks_assert(config);

	ks_json_add_bool_to_object(config->network, "route_add", value);
}

SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_route_remove(swclt_config_t *config)
{
	ks_assert(config);

	return ks_json_get_object_bool(config->network, "route_remove", KS_FALSE);
}

SWCLT_DECLARE(void) swclt_config_set_network_route_remove(swclt_config_t *config, ks_bool_t value)
{
	ks_assert(config);

	ks_json_add_bool_to_object(config->network, "route_remove", value);
}

SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_authority_data(swclt_config_t *config)
{
	ks_assert(config);

	return ks_json_get_object_bool(config->network, "authority_data", KS_FALSE);
}

SWCLT_DECLARE(void) swclt_config_set_network_authority_data(swclt_config_t *config, ks_bool_t value)
{
	ks_assert(config);

	ks_json_add_bool_to_object(config->network, "authority_data", value);
}

SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_authority_add(swclt_config_t *config)
{
	ks_assert(config);

	return ks_json_get_object_bool(config->network, "authority_add", KS_FALSE);
}

SWCLT_DECLARE(void) swclt_config_set_network_authority_add(swclt_config_t *config, ks_bool_t value)
{
	ks_assert(config);

	ks_json_add_bool_to_object(config->network, "authority_add", value);
}

SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_authority_remove(swclt_config_t *config)
{
	ks_assert(config);

	return ks_json_get_object_bool(config->network, "authority_remove", KS_FALSE);
}

SWCLT_DECLARE(void) swclt_config_set_network_authority_remove(swclt_config_t *config, ks_bool_t value)
{
	ks_assert(config);

	ks_json_add_bool_to_object(config->network, "authority_remove", value);
}

SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_filtered_protocols(swclt_config_t *config)
{
	ks_assert(config);

	return ks_json_get_object_bool(config->network, "filtered_protocols", KS_FALSE);
}

SWCLT_DECLARE(void) swclt_config_set_network_filtered_protocols(swclt_config_t *config, ks_bool_t value)
{
	ks_assert(config);

	ks_json_add_bool_to_object(config->network, "filtered_protocols", value);
}

SWCLT_DECLARE(void) swclt_config_add_network_protocol(swclt_config_t *config, const char *value)
{
	ks_json_t *protocols = NULL;
	ks_assert(config);

	if (!(protocols = ks_json_get_object_item(config->network, "protocols"))) {
		protocols = ks_json_add_array_to_object(config->network, "protocols");
	}

	for (int index = 0; index < ks_json_get_array_size(protocols); ++index) {
		if (!strcmp(ks_json_get_array_string(protocols, index, NULL), value)) return;
	}

	ks_json_add_string_to_array(protocols, value);
}

SWCLT_DECLARE(void) swclt_config_remove_network_protocol(swclt_config_t *config, const char *value)
{
	ks_json_t *protocols = NULL;
	ks_assert(config);

	if (!(protocols = ks_json_get_object_item(config->network, "protocols"))) {
		protocols = ks_json_add_array_to_object(config->network, "protocols");
	}

	for (int index = 0; index < ks_json_get_array_size(protocols); ++index) {
		if (!strcmp(ks_json_get_array_string(protocols, index, NULL), value)) {
			ks_json_delete_item_from_array(protocols, index);
			return;
		}
	}
}

