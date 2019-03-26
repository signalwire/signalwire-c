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

	pool = ks_pool_get(*config);
	ks_pool_close(&pool);

	*config = NULL;

	return ret;
}

SWCLT_DECLARE(ks_status_t) swclt_config_load_from_json(swclt_config_t *config, ks_json_t *json)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	const char *val = NULL;

	if ((val = ks_json_get_object_cstr_def(json, "private_key_path", NULL))) {
		swclt_config_set_private_key_path(config, val);
	}

	if ((val = ks_json_get_object_cstr_def(json, "client_cert_path", NULL))) {
		swclt_config_set_client_cert_path(config, val);
	}

	if ((val = ks_json_get_object_cstr_def(json, "cert_chain_path", NULL))) {
		swclt_config_set_cert_chain_path(config, val);
	}
	
	if ((val = ks_json_get_object_cstr_def(json, "authentication", NULL))) {
		swclt_config_set_authentication(config, val);
	}
	
	return ret;
}

SWCLT_DECLARE(ks_status_t) swclt_config_load_from_env(swclt_config_t *config)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	const char *val = NULL;

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

