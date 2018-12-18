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
#include "swclt_test.h"

static SWCLT_HSTATE g_last_state_change;
static ks_cond_t *g_cond;
static char *g_message_id;
static ks_bool_t g_messaging_event_received;

static void __on_sess_hmon_event(swclt_sess_t sess, swclt_hstate_change_t *state_change_info, const char *cb_data)
{
	REQUIRE(!strcmp(cb_data, "bobo"));
	ks_cond_lock(g_cond);
	g_last_state_change = state_change_info->new_state;
	ks_cond_broadcast(g_cond);
	ks_cond_unlock(g_cond);
}

typedef void(*swclt_hstate_change_cb_t)(swclt_handle_base_t *ctx, swclt_hstate_change_t *state_change_request);

static void __on_messaging_events(swclt_sess_t sess, blade_broadcast_rqu_t *rqu, void *cb_data)
{
	ks_log(KS_LOG_INFO, "Messaging Event Handler!");
	g_messaging_event_received = KS_TRUE;
}

void test_messaging_exp(ks_pool_t *pool)
{
	swclt_sess_t sess;
	swclt_sess_ctx_t *sess_ctx;
	swclt_hmon_t hmon;
	const char *authentication, *cert_chain_path;
	const char *ident;
	swclt_cmd_t cmd;
	SWCLT_CMD_TYPE cmd_type;
	const ks_json_t *result;
	signalwire_messaging_send_response_t *send_res;
	signalwire_messaging_status_response_t *status_res;

	REQUIRE(!ks_cond_create(&g_cond, NULL));

	/* Load the config we expect session to load */
	REQUIRE(swclt_config_get_authentication(g_uncertified_config));

	REQUIRE(!swclt_sess_create(&sess, g_target_ident_str, g_uncertified_config));

	/* Register a monitor to get to know when session comes online successfully */
	REQUIRE(!swclt_hmon_register(&hmon, sess, __on_sess_hmon_event, "bobo"));

	{
		ks_handle_t next = 0;
		uint32_t count = 0;

		while (!ks_handle_enum_type(SWCLT_HTYPE_HMON, &next))
			count++;

		REQUIRE(count == 1);
	}

	ks_cond_lock(g_cond);

	/* Now take the session onine */
	REQUIRE(!swclt_sess_connect(sess));

	ks_cond_wait(g_cond);

	/* Should be online */
	REQUIRE(g_last_state_change == SWCLT_HSTATE_ONLINE);

	REQUIRE(!swclt_sess_messaging_setup(sess, __on_messaging_events, NULL));

	/* Send a sync SMS send request, blocking until we get the response */
	REQUIRE(!swclt_sess_messaging_send(sess, "+12622081318", "+15559999999", "Hello World! Brought to you by SignalWire!", NULL, &cmd));

	/* Make sure the command handle is valid */
	REQUIRE(ks_handle_valid(cmd));

	/* Get the command type, make sure it's valid */
	REQUIRE(!swclt_cmd_type(cmd, &cmd_type));

	/* Make sure it's a result, not an error */
	REQUIRE(cmd_type == SWCLT_CMD_TYPE_RESULT);

	/* Check that there is a result */
	REQUIRE(!swclt_cmd_result(cmd, &result));

	REQUIRE(result = ks_json_get_object_item(result, "result"));

	/* Parse the response result, allocate in the command's pool so it's destroyed with it (good form) */
	REQUIRE(!SIGNALWIRE_MESSAGING_SEND_RESPONSE_PARSE(ks_handle_pool(cmd), result, &send_res));

	ks_log(KS_LOG_INFO, "Received send response message id %s, status %s\n", send_res->id, send_res->status);

	/* Make a copy of the send_res->id allocated in the session pool so it won't be cleaned up until we're done with it */
	g_message_id = ks_pstrdup(ks_handle_pool(sess), send_res->id);

	/* Confirm we actually have a message id */
	REQUIRE(g_message_id && g_message_id[0]);

	/* Cleanup the parsed response result, even though it would be cleaned up with the command (good form) */
	SIGNALWIRE_MESSAGING_SEND_RESPONSE_DESTROY(&send_res);

	/* Cleanup the command that we kept allocated */
	ks_handle_destroy(&cmd);

	
	/* Send a sync SMS status request, blocking until we get the response */
	REQUIRE(!swclt_sess_messaging_status(sess, g_message_id, &cmd));

	/* Make sure the command handle is valid */
	REQUIRE(ks_handle_valid(cmd));

	/* Get the command type, make sure it's valid */
	REQUIRE(!swclt_cmd_type(cmd, &cmd_type));

	/* Make sure it's a result, not an error */
	REQUIRE(cmd_type == SWCLT_CMD_TYPE_RESULT);

	/* Check that there is a result */
	REQUIRE(!swclt_cmd_result(cmd, &result));

	REQUIRE(result = ks_json_get_object_item(result, "result"));
	
	/* Parse the response result, allocate in the command's pool so it's destroyed with it (good form) */
	REQUIRE(!SIGNALWIRE_MESSAGING_STATUS_RESPONSE_PARSE(ks_handle_pool(cmd), result, &status_res));

	ks_log(KS_LOG_INFO, "Received status response message id %s, status %s\n", status_res->id, status_res->status);

	/* Cleanup the parsed response result, even though it would be cleaned up with the command (good form) */
	SIGNALWIRE_MESSAGING_STATUS_RESPONSE_DESTROY(&status_res);

	/* Cleanup the command that we kept allocated */
	ks_handle_destroy(&cmd);

	/* Sleep for 15 seconds to receive any events that are delayed */
	ks_sleep_ms(15000);

	/* Confirm that we received at least 1 messaging event (status update) in 15 seconds */
	REQUIRE(g_messaging_event_received);
	
	/* Now disconnect the session */
	REQUIRE(!swclt_sess_disconnect(sess));

	/* Should be offline now */
	ks_cond_wait(g_cond);

	REQUIRE(g_last_state_change == SWCLT_HSTATE_OFFLINE);

	/* Now delete the monitor */
	ks_handle_destroy(&hmon);

	ks_cond_unlock(g_cond);

	/* Ok we're done */
	ks_cond_destroy(&g_cond);
	ks_handle_destroy(&sess);
}
