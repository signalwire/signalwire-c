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

static void __on_session_state(swclt_sess_t sess, const swclt_hstate_change_t *state_change_info, ks_cond_t *cond)
{
	/* Notify the waiting test of the state change */
	ks_cond_lock(cond);
	ks_cond_broadcast(cond);
	ks_cond_unlock(cond);
}

static ks_status_t __on_incoming_test_execute_rqu(swclt_sess_t sess, swclt_cmd_t cmd, const blade_execute_rqu_t *rqu, void *data)
{
	/* Formulate a response */
	ks_cond_t *cond = (ks_cond_t *)data;
	ks_pool_t *cmd_pool = ks_handle_pool(cmd);
	ks_json_t *result = ks_json_pcreate_object(cmd_pool);
	ks_json_padd_string_to_object(cmd_pool, result, "reply", "i got it!");

	ks_json_t *cmd_result = BLADE_EXECUTE_RPL_MARSHAL(
		cmd_pool,
		&(blade_execute_rpl_t){
			rqu->requester_nodeid,
			rqu->responder_nodeid,
			result});

	REQUIRE(!swclt_cmd_set_result(cmd, &cmd_result));

	/* Notify the waiting test of the state change */
	ks_cond_lock(cond);
	ks_cond_broadcast(cond);
	ks_cond_unlock(cond);
	return KS_STATUS_SUCCESS;
}

static void __on_outgoing_test_execute_rpl(swclt_cmd_t cmd, ks_cond_t *cond)
{
	ks_cond_lock(cond);
	ks_cond_broadcast(cond);
	ks_cond_unlock(cond);
}

void test_execute(ks_pool_t *pool)
{
	swclt_sess_t sess1, sess2;
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
	REQUIRE(!swclt_hmon_register(&sess1_mon, sess1, __on_session_state, cond));
	REQUIRE(!swclt_hmon_register(&sess2_mon, sess2, __on_session_state, cond));

	/* On the second session register a execute handler we'll communicate with */
	REQUIRE(!swclt_sess_register_protocol_method(sess2, "test", "test.method", __on_incoming_test_execute_rqu, cond));

	/* Initiate the connect */
	REQUIRE(!swclt_sess_connect(sess1));
	REQUIRE(!swclt_sess_connect(sess2));

	/* Wait for the state to change, twice */
	ks_cond_wait(cond);
	ks_cond_wait(cond);

	/* Load our nodeids for both */
	REQUIRE(!swclt_sess_info(sess1, pool, NULL, &nodeid1, NULL));
	REQUIRE(!swclt_sess_info(sess2, pool, NULL, &nodeid2, NULL));

	/* Now execute from the first session to the second session */
	ks_json_t *params = ks_json_create_object();
	swclt_cmd_t sess1_exec_cmd;
	REQUIRE(params);
	REQUIRE(ks_json_add_string_to_object(params, "arg", "value"));
	REQUIRE(!swclt_sess_execute_async(sess1, nodeid2, "test", "test.method", &params, (swclt_cmd_cb_t)__on_outgoing_test_execute_rpl, cond, &sess1_exec_cmd));
	ks_cond_wait(cond);
	ks_cond_wait(cond);
	SWCLT_CMD_TYPE type;
	REQUIRE(!swclt_cmd_type(sess1_exec_cmd, &type));
	ks_log(KS_LOG_INFO, "Final type is: %s", swclt_cmd_type_str(type));
	REQUIRE(type == SWCLT_CMD_TYPE_RESULT);

	ks_handle_destroy(&sess1_mon);
	ks_handle_destroy(&sess2_mon);

	ks_handle_destroy(&sess1);
	ks_handle_destroy(&sess2);
	ks_handle_destroy(&cfg);
	ks_cond_destroy(&cond);
}
