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

/* Obfuscate our session internals */
typedef struct swclt_sess_ctx swclt_sess_ctx_t;

typedef void (*swclt_sess_auth_failed_cb_t)(swclt_sess_t sess);

SWCLT_DECLARE(ks_status_t) __swclt_sess_create(
	swclt_sess_t *sess,
	const char *identity_uri,
	swclt_config_t *config,
   	const char *file,
   	int line,
   	const char *tag);

#define swclt_sess_create(sess, ident, cfg) __swclt_sess_create(sess, ident, cfg, __FILE__, __LINE__, __PRETTY_FUNCTION__)

SWCLT_DECLARE(ks_status_t) swclt_sess_set_auth_failed_cb(swclt_sess_t sess, swclt_sess_auth_failed_cb_t cb);
SWCLT_DECLARE(ks_status_t) swclt_sess_target_set(swclt_sess_t sess, const char *target);

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_register(swclt_sess_t sess, const char *protocol, int interval, int rank);
SWCLT_DECLARE(ks_status_t) swclt_sess_metric_update(swclt_sess_t sess, const char *protocol, int rank);
SWCLT_DECLARE(ks_status_t) swclt_sess_metric_current(swclt_sess_t sess, const char *protocol, int *rank);

SWCLT_DECLARE(ks_bool_t) swclt_sess_has_authentication(swclt_sess_t sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_rescan_env_config(swclt_sess_t sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_connect(swclt_sess_t sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_disconnect(swclt_sess_t sess);
SWCLT_DECLARE(ks_bool_t) swclt_sess_connected(swclt_sess_t sess);
SWCLT_DECLARE(ks_bool_t) swclt_sess_restored(swclt_sess_t sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_info(swclt_sess_t sess, ks_pool_t *pool, ks_uuid_t *sessionid, char **nodeid, char **master_nodeid);
SWCLT_DECLARE(ks_status_t) swclt_sess_nodeid(swclt_sess_t sess, ks_pool_t *pool, char **nodeid);
SWCLT_DECLARE(ks_bool_t) swclt_sess_nodeid_local(swclt_sess_t sess, const char *nodeid);

SWCLT_DECLARE(ks_status_t) __swclt_sess_register_protocol_method(
	swclt_sess_t sess,
	const char *protocol,
	const char *method,
	swclt_pmethod_cb_t pmethod,
	void *cb_data);
#define swclt_sess_register_protocol_method(sess, protocol, method, pmethod, cb_data) \
	__swclt_sess_register_protocol_method(sess, protocol, method, pmethod, cb_data)

SWCLT_DECLARE(ks_status_t) swclt_sess_register_subscription_method(
	swclt_sess_t sess,
	swclt_sub_t *sub,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_sess_broadcast(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	const char *event,
	ks_json_t **params);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_sub_t *sub,
	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add_async(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_sub_t *sub,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove_async(
	swclt_sess_t sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmd);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add(
	swclt_sess_t sess,
	const char * protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
	ks_json_t **channels,
	int rank,
	ks_json_t **data,
	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add_async(
	swclt_sess_t sess,
	const char * protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
	ks_json_t **channels,
	int rank,
	ks_json_t **data,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmd);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove(swclt_sess_t sess, const char * protocol, swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove_async(
	swclt_sess_t sess,
	const char * protocol,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update(
	swclt_sess_t sess,
	const char * protocol,
	int rank,
	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update_async(
	swclt_sess_t sess,
	const char * protocol,
	int rank,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmd);

SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add(swclt_sess_t sess, const char *identity, swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add_async(
	swclt_sess_t sess,
	const char *identity,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmd);

SWCLT_DECLARE(ks_status_t) swclt_sess_execute(
	swclt_sess_t sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
   	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_nodestore(swclt_sess_t sess, swclt_store_t *store);

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_async(
	swclt_sess_t sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_t *cmdP);

SWCLT_DECLARE(ks_status_t) swclt_sess_nodestore(swclt_sess_t sess, swclt_store_t *store);
SWCLT_DECLARE(ks_status_t) swclt_sess_get_rates(swclt_sess_t sess, ks_throughput_t *recv, ks_throughput_t *send);

#define swclt_sess_get(sess, contextP)		__ks_handle_get(SWCLT_HTYPE_SESS, sess, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_sess_put(contextP)			__ks_handle_put(SWCLT_HTYPE_SESS, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)

SWCLT_DECLARE(ks_status_t) swclt_sess_signalwire_setup(swclt_sess_t sess, const char *service, swclt_sub_cb_t cb, void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_setup(swclt_sess_t sess, swclt_sub_cb_t cb, void *cb_data);
													  
SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure(swclt_sess_t sess,
															 const char *target,
															 const char *local_endpoint,
															 const char *external_endpoint,
															 const char *relay_connector_id,
															 swclt_cmd_t *cmdP);
SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure_async(swclt_sess_t sess,
																   const char *target,
																   const char *local_endpoint,
																   const char *external_endpoint,
																   const char *relay_connector_id,
																   swclt_cmd_cb_t response_callback,
																   void *response_callback_data,
																   swclt_cmd_t *cmdP);

KS_END_EXTERN_C

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
