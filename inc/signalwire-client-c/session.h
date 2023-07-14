/*
 * Copyright (c) 2018-2022 SignalWire, Inc
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

typedef enum swclt_sess_state {
	SWCLT_STATE_OFFLINE,
	SWCLT_STATE_ONLINE,
	SWCLT_STATE_RESTORED
} swclt_sess_state_t;

static inline const char *swclt_sess_state_str(swclt_sess_state_t state)
{
	switch (state) {
		case SWCLT_STATE_OFFLINE:
			return "None";
		case SWCLT_STATE_ONLINE:
			return "Online";
		case SWCLT_STATE_RESTORED:
			return "Restored";
		default:
			return "";
	}
}

typedef void (*swclt_sess_auth_failed_cb_t)(swclt_sess_t *sess);
typedef void (*swclt_sess_state_change_cb_t)(swclt_sess_t *sess, void *cb_data);

struct swclt_result_queue;
typedef struct swclt_result_queue swclt_result_queue_t;

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
struct swclt_sess {
	/* The pool that manages all the allocations */
	ks_pool_t *pool;

	/* The node store, an api to keep the cache in sync with blade, contains
	 * a lotta info about stuff. */
	swclt_store_t *store;

	/* Our connection */
	swclt_conn_t *conn;

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

	/* Setups completed for provider event channels */
	ks_hash_t *setups;

	/* Registry for metric rank updates for local protocols */
	ks_hash_t *metrics;

	/* Results that are pending delivery on re-connection */
	swclt_result_queue_t *result_first;
	swclt_result_queue_t *result_last;
	ks_mutex_t *result_mutex;

	/* Optional callback for state changes */
	swclt_sess_state_change_cb_t state_change_cb;
	void *state_change_cb_data;
	swclt_sess_state_t state;
	ks_time_t disconnect_time;
	ks_time_t connect_time;
	ks_cond_t *monitor_cond;
	ks_thread_t *monitor_thread;

	ks_rwl_t *rwlock;
};

SWCLT_DECLARE(ks_status_t) swclt_sess_create(
	swclt_sess_t **sess,
	const char *identity_uri,
	swclt_config_t *config);

SWCLT_DECLARE(ks_status_t) swclt_sess_destroy(swclt_sess_t **sess);

SWCLT_DECLARE(ks_status_t) swclt_sess_set_auth_failed_cb(swclt_sess_t *sess, swclt_sess_auth_failed_cb_t cb);
SWCLT_DECLARE(ks_status_t) swclt_sess_set_state_change_cb(swclt_sess_t *sess, swclt_sess_state_change_cb_t cb, void *cb_data);
SWCLT_DECLARE(ks_status_t) swclt_sess_target_set(swclt_sess_t *sess, const char *target);

SWCLT_DECLARE(ks_status_t) swclt_sess_metric_register(swclt_sess_t *sess, const char *protocol, int interval, int rank);
SWCLT_DECLARE(ks_status_t) swclt_sess_metric_update(swclt_sess_t *sess, const char *protocol, int rank);
SWCLT_DECLARE(ks_status_t) swclt_sess_metric_current(swclt_sess_t *sess, const char *protocol, int *rank);

SWCLT_DECLARE(ks_bool_t) swclt_sess_has_authentication(swclt_sess_t *sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_rescan_env_config(swclt_sess_t *sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_connect(swclt_sess_t *sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_disconnect(swclt_sess_t *sess);
SWCLT_DECLARE(ks_bool_t) swclt_sess_connected(swclt_sess_t *sess);
SWCLT_DECLARE(ks_bool_t) swclt_sess_restored(swclt_sess_t *sess);
SWCLT_DECLARE(ks_status_t) swclt_sess_info(swclt_sess_t *sess, ks_pool_t *pool, ks_uuid_t *sessionid, char **nodeid, char **master_nodeid);
SWCLT_DECLARE(ks_status_t) swclt_sess_nodeid(swclt_sess_t *sess, ks_pool_t *pool, char **nodeid);
SWCLT_DECLARE(ks_bool_t) swclt_sess_nodeid_local(swclt_sess_t *sess, const char *nodeid);
SWCLT_DECLARE(ks_status_t) swclt_sess_wait_for_cmd_reply(swclt_sess_t *sess, swclt_cmd_future_t **future, swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_register_protocol_method(
	swclt_sess_t *sess,
	const char *protocol,
	const char *method,
	swclt_pmethod_cb_t pmethod,
	void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_sess_register_subscription_method(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_sess_broadcast(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	const char *event,
	ks_json_t **params);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_add_async(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_sub_cb_t cb,
	void *cb_data,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_subscription_remove_async(
	swclt_sess_t *sess,
	const char *protocol,
	const char *channel,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add(
	swclt_sess_t *sess,
	const char * protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
	ks_json_t **channels,
	int rank,
	ks_json_t **data,
	swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_add_async(
	swclt_sess_t *sess,
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
	swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove(swclt_sess_t *sess, const char * protocol, swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_remove_async(
	swclt_sess_t *sess,
	const char * protocol,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update(
	swclt_sess_t *sess,
	const char * protocol,
	int rank,
	swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_protocol_provider_rank_update_async(
	swclt_sess_t *sess,
	const char * protocol,
	int rank,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add(swclt_sess_t *sess, const char *identity, swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_identity_add_async(
	swclt_sess_t *sess,
	const char *identity,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_sess_execute(
	swclt_sess_t *sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_with_id(
	swclt_sess_t *sess,
	const char *id,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_reply_t **reply);

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_async(
	swclt_sess_t *sess,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_sess_execute_with_id_async(
	swclt_sess_t *sess,
	const char *id,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params,
	swclt_cmd_cb_t response_callback,
	void *response_callback_data,
	swclt_cmd_future_t **future);
						   
SWCLT_DECLARE(ks_status_t) swclt_sess_signalwire_setup(swclt_sess_t *sess, const char *service, swclt_sub_cb_t cb, void *cb_data);
SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_setup(swclt_sess_t *sess, swclt_sub_cb_t cb, void *cb_data);
													  
SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure(swclt_sess_t *sess,
															 const char *target,
															 const char *local_endpoint,
															 const char *external_endpoint,
															 const char *relay_connector_id,
															 swclt_cmd_reply_t **reply);
SWCLT_DECLARE(ks_status_t) swclt_sess_provisioning_configure_async(swclt_sess_t *sess,
																   const char *target,
																   const char *local_endpoint,
																   const char *external_endpoint,
																   const char *relay_connector_id,
																   swclt_cmd_cb_t response_callback,
																   void *response_callback_data,
																   swclt_cmd_future_t **future);

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
