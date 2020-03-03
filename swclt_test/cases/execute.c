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

#include "swclt_test.h"

static void __on_session_state_provider(swclt_sess_t *sess, swclt_hstate_change_t *state_change_info, ks_cond_t *cond)
{
	swclt_sess_state_t old_state = state_change_info->old_state;
	swclt_sess_state_t new_state = state_change_info->new_state;
	if (new_state == SWCLT_STATE_ONLINE && old_state == SWCLT_STATE_NORMAL) {
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
	} else if (new_state == SWCLT_STATE_OFFLINE) {
		// Disconnected
	}

	/* Notify the waiting test of the state change */
	ks_cond_broadcast(cond);
}

static void __on_session_state(swclt_sess_t *sess, const swclt_hstate_change_t *state_change_info, ks_cond_t *cond)
{
	/* Notify the waiting test of the state change */
	ks_cond_broadcast(cond);
}

static ks_status_t __on_incoming_test_execute_rqu(swclt_sess_t *sess, swclt_cmd_t *cmd, const blade_execute_rqu_t *rqu, void *data)
{
	/* Formulate a response */
	ks_cond_t *cond = (ks_cond_t *)data;
	ks_pool_t *cmd_pool = cmd->pool;
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

static void __on_outgoing_test_execute_rpl(swclt_cmd_reply_t *reply, void *cond)
{
	ks_cond_broadcast((ks_cond_t *)cond);
}

void test_execute(ks_pool_t *pool)
{
	swclt_sess_t *sess1 = NULL, *sess2 = NULL;
	swclt_cfg_t cfg;
	char *nodeid1, *nodeid2;
	swclt_hmon_t sess1_mon, sess2_mon;
	ks_cond_t *cond;
	const char *ident;

	REQUIRE(!ks_cond_create(&cond, pool));
	ks_cond_lock(cond);

	/* Create two sessions to blade */
	REQUIRE(!swclt_sess_create(&sess1, g_target_ident_str, g_certified_config));
	REQUIRE(!swclt_sess_create(&sess2, g_target_ident_str, g_certified_config));

	/* Get called back when they're connected */
	// TODO fix this
	//REQUIRE(!swclt_hmon_register(&sess1_mon, sess1, __on_session_state, cond));
	//REQUIRE(!swclt_hmon_register(&sess2_mon, sess2, __on_session_state_provider, cond));

	/* On the second session register a execute handler we'll communicate with */
	REQUIRE(!swclt_sess_register_protocol_method(sess2, "test", "test.method", __on_incoming_test_execute_rqu, cond));

	/* Initiate the connect */
	REQUIRE(!swclt_sess_connect(sess1));
	REQUIRE(!swclt_sess_connect(sess2));

	ks_cond_wait(cond);
	ks_cond_wait(cond);

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
	REQUIRE(!swclt_cmd_future_get(future, &reply));
	swclt_cmd_future_destroy(&future);
	swclt_cmd_reply_destroy(&reply);

	/* repeat, but this time don't wait for any response */
	params = ks_json_create_object();
	REQUIRE(params);
	ks_json_add_string_to_object(params, "arg", "value");
	REQUIRE(!swclt_sess_execute_async(sess1, nodeid2, "test", "test.method", &params, NULL, NULL, NULL));

	ks_sleep_ms(5000);

	//ks_handle_destroy(&sess1_mon);
	//ks_handle_destroy(&sess2_mon);

	swclt_sess_destroy(&sess1);
	swclt_sess_destroy(&sess2);
	ks_handle_destroy(&cfg);
	ks_cond_destroy(&cond);
}