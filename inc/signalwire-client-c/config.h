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

KS_BEGIN_EXTERN_C

/* Obfuscate our config internals */
typedef struct swclt_config_s swclt_config_t;

SWCLT_DECLARE(ks_status_t) swclt_config_create(swclt_config_t **config);
SWCLT_DECLARE(ks_status_t) swclt_config_destroy(swclt_config_t **config);

SWCLT_DECLARE(ks_status_t) swclt_config_load_from_json(swclt_config_t *config, ks_json_t *json);
SWCLT_DECLARE(ks_status_t) swclt_config_load_from_env(swclt_config_t *config);

SWCLT_DECLARE(const char *) swclt_config_get_private_key_path(swclt_config_t *config);
SWCLT_DECLARE(ks_status_t) swclt_config_set_private_key_path(swclt_config_t *config, const char *value);
SWCLT_DECLARE(const char *) swclt_config_get_client_cert_path(swclt_config_t *config);
SWCLT_DECLARE(ks_status_t) swclt_config_set_client_cert_path(swclt_config_t *config, const char *value);
SWCLT_DECLARE(const char *) swclt_config_get_cert_chain_path(swclt_config_t *config);
SWCLT_DECLARE(ks_status_t) swclt_config_set_cert_chain_path(swclt_config_t *config, const char *value);
SWCLT_DECLARE(const char *) swclt_config_get_authentication(swclt_config_t *config);
SWCLT_DECLARE(ks_status_t) swclt_config_set_authentication(swclt_config_t *config, const char *value);
SWCLT_DECLARE(const char *) swclt_config_get_agent(swclt_config_t *config);
SWCLT_DECLARE(ks_status_t) swclt_config_set_agent(swclt_config_t *config, const char *value);
SWCLT_DECLARE(const char *) swclt_config_get_identity(swclt_config_t *config);
SWCLT_DECLARE(ks_status_t) swclt_config_set_identity(swclt_config_t *config, const char *value);

SWCLT_DECLARE(ks_status_t) swclt_config_set_default_network(swclt_config_t *config, ks_bool_t allData);
SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_route_data(swclt_config_t *config);
SWCLT_DECLARE(void) swclt_config_set_network_route_data(swclt_config_t *config, ks_bool_t value);
SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_route_add(swclt_config_t *config);
SWCLT_DECLARE(void) swclt_config_set_network_route_add(swclt_config_t *config, ks_bool_t value);
SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_route_remove(swclt_config_t *config);
SWCLT_DECLARE(void) swclt_config_set_network_route_remove(swclt_config_t *config, ks_bool_t value);
SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_authority_data(swclt_config_t *config);
SWCLT_DECLARE(void) swclt_config_set_network_authority_data(swclt_config_t *config, ks_bool_t value);
SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_authority_add(swclt_config_t *config);
SWCLT_DECLARE(void) swclt_config_set_network_authority_add(swclt_config_t *config, ks_bool_t value);
SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_authority_remove(swclt_config_t *config);
SWCLT_DECLARE(void) swclt_config_set_network_authority_remove(swclt_config_t *config, ks_bool_t value);
SWCLT_DECLARE(ks_bool_t) swclt_config_get_network_filtered_protocols(swclt_config_t *config);
SWCLT_DECLARE(void) swclt_config_set_network_filtered_protocols(swclt_config_t *config, ks_bool_t value);
SWCLT_DECLARE(void) swclt_config_add_network_protocol(swclt_config_t *config, const char *value);
SWCLT_DECLARE(void) swclt_config_remove_network_protocol(swclt_config_t *config, const char *value);

KS_END_EXTERN_C
