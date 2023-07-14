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

#include "swclt_test.h"

static void __on_session_state_provider(swclt_sess_t *sess, void *condV)
{
	ks_cond_t *cond = (ks_cond_t *)condV;
	if (sess->state == SWCLT_STATE_ONLINE) {
		ks_json_t *channels = ks_json_create_array();
		swclt_sess_protocol_provider_add(sess,
										 "test",
										 BLADE_ACL_SYSTEM, // rpc execute
										 BLADE_ACL_SYSTEM, // channel subscribe
										 BLADE_ACL_SYSTEM, // channel broadcast
										 NULL, // method access overrides
										 &channels,
										 3,
										 NULL,
										 NULL);

		swclt_sess_metric_register(sess, "test", 30, 9);
	} else if (sess->state == SWCLT_STATE_OFFLINE) {
		// Disconnected
	}

	/* Notify the waiting test of the state change */
	ks_cond_lock(cond);
	ks_cond_broadcast(cond);
	ks_cond_unlock(cond);
}

static void __on_session_state(swclt_sess_t *sess, void *condV)
{
	/* Notify the waiting test of the state change */
	ks_cond_t *cond = (ks_cond_t *)condV;
	ks_cond_lock(cond);
	ks_cond_broadcast(cond);
	ks_cond_unlock(cond);
}

static ks_status_t __on_incoming_test_execute_rqu(swclt_sess_t *sess, swclt_cmd_t *cmd, const blade_execute_rqu_t *rqu, void *data)
{
	/* Formulate a response */
	ks_cond_t *cond = (ks_cond_t *)data;
	ks_json_t *result = ks_json_create_object();
	ks_json_add_string_to_object(result, "reply", "i got it!");

	ks_json_t *cmd_result = BLADE_EXECUTE_RPL_MARSHAL(
		&(blade_execute_rpl_t){
			rqu->requester_nodeid,
			rqu->responder_nodeid,
			result});

	REQUIRE(!swclt_cmd_set_result(cmd, &cmd_result));

	return KS_STATUS_SUCCESS;
}


static ks_status_t __on_incoming_test_execute_rqu_slow(swclt_sess_t *sess, swclt_cmd_t *cmd, const blade_execute_rqu_t *rqu, void *data)
{
	/* Formulate a response */
	ks_cond_t *cond = (ks_cond_t *)data;
	ks_json_t *result = ks_json_create_object();
	ks_json_add_string_to_object(result, "reply", "pong!");

	ks_json_t *cmd_result = BLADE_EXECUTE_RPL_MARSHAL(
		&(blade_execute_rpl_t){
			rqu->requester_nodeid,
			rqu->responder_nodeid,
			result});

	ks_sleep_ms(1000);

	REQUIRE(!swclt_cmd_set_result(cmd, &cmd_result));

	return KS_STATUS_SUCCESS;
}

static void __on_outgoing_test_execute_rpl(swclt_cmd_reply_t *reply, void *cond)
{
	ks_cond_broadcast((ks_cond_t *)cond);
}

void test_execute(ks_pool_t *pool)
{
	swclt_sess_t *sess1 = NULL, *sess2 = NULL;
	char *nodeid1, *nodeid2;
	ks_cond_t *cond;
	const char *ident;

	REQUIRE(!ks_cond_create(&cond, pool));
	ks_cond_lock(cond);

	/* Create two sessions to blade */
	REQUIRE(!swclt_sess_create(&sess1, g_target_ident_str, g_certified_config));
	REQUIRE(!swclt_sess_create(&sess2, g_target_ident_str, g_certified_config));

	/* Get called back when they're connected */
	REQUIRE(!swclt_sess_set_state_change_cb(sess1, __on_session_state, cond));
	REQUIRE(!swclt_sess_set_state_change_cb(sess2, __on_session_state_provider, cond));

	/* On the second session register a execute handler we'll communicate with */
	REQUIRE(!swclt_sess_register_protocol_method(sess2, "test", "test.method", __on_incoming_test_execute_rqu, cond));
	REQUIRE(!swclt_sess_register_protocol_method(sess2, "test", "test.slow_method", __on_incoming_test_execute_rqu_slow, cond));

	/* Initiate the connect */
	REQUIRE(!swclt_sess_connect(sess1));
	REQUIRE(!swclt_sess_connect(sess2));

	int i = 15;
	while (i-- > 0 && (!swclt_sess_connected(sess1) || !swclt_sess_connected(sess2))) {
		ks_cond_timedwait(cond, 1000);
	}
	REQUIRE(swclt_sess_connected(sess1));
	REQUIRE(swclt_sess_connected(sess2));

	ks_cond_unlock(cond);

	/* Load our nodeids for both */
	REQUIRE(!swclt_sess_info(sess1, pool, NULL, &nodeid1, NULL));
	REQUIRE(!swclt_sess_info(sess2, pool, NULL, &nodeid2, NULL));

	/* Now execute from the first session to the second session */
	ks_json_t *params = ks_json_create_object();
	REQUIRE(params);
	ks_json_add_string_to_object(params, "arg", "value");
	swclt_cmd_future_t *future = NULL;
	REQUIRE(!swclt_sess_execute_async(sess1, nodeid2, "test", "test.method", &params, NULL, NULL, &future));
	REQUIRE(future);
	swclt_cmd_reply_t *reply = NULL;
	REQUIRE(!swclt_sess_wait_for_cmd_reply(sess1, &future, &reply));
	swclt_cmd_reply_destroy(&reply);

	/* repeat, but this time don't wait for any response */
	params = ks_json_create_object();
	REQUIRE(params);
	ks_json_add_string_to_object(params, "arg", "value");
	REQUIRE(!swclt_sess_execute_async(sess1, nodeid2, "test", "test.method", &params, NULL, NULL, NULL));

	ks_sleep_ms(5000);

	/* Now execute from the first session to the second session forcing a disconnect before the response is delivered */
	params = ks_json_create_object();
	REQUIRE(params);
	ks_json_add_string_to_object(params, "arg", "value");
	future = NULL;
	REQUIRE(!swclt_sess_execute_async(sess1, nodeid2, "test", "test.slow_method", &params, NULL, NULL, &future));
	REQUIRE(future);

	ks_sleep_ms(200);

	/* Disconnect server */
	swclt_sess_disconnect(sess2);

	ks_sleep_ms(2000);

	/* Reconnect server */
	swclt_sess_connect(sess2);

	reply = NULL;
	REQUIRE(!swclt_sess_wait_for_cmd_reply(sess1, &future, &reply));
	swclt_cmd_reply_destroy(&reply);

	swclt_sess_destroy(&sess1);
	swclt_sess_destroy(&sess2);
	ks_cond_destroy(&cond);
}
