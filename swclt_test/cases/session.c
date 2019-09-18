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

static void __on_sess_hmon_event(swclt_sess_t sess, swclt_hstate_change_t *state_change_info, const char *cb_data)
{
	REQUIRE(!strcmp(cb_data, "bobo"));
	ks_cond_lock(g_cond);
	g_last_state_change = state_change_info->new_state;
	ks_cond_broadcast(g_cond);
	ks_cond_unlock(g_cond);
}

typedef void(*swclt_hstate_change_cb_t)(swclt_handle_base_t *ctx, swclt_hstate_change_t *state_change_request);

void test_session(ks_pool_t *pool)
{
	swclt_sess_t sess;
	swclt_sess_ctx_t *sess_ctx;
	swclt_hmon_t hmon;

	REQUIRE(!ks_cond_create(&g_cond, NULL));

	/* Load the config we expect session to load */
	REQUIRE(swclt_config_get_private_key_path(g_certified_config));
	REQUIRE(swclt_config_get_client_cert_path(g_certified_config));
	REQUIRE(swclt_config_get_cert_chain_path(g_certified_config));

	REQUIRE(!swclt_sess_create(&sess, g_target_ident_str, g_certified_config));

	/* Register a monitor to get to know when session comes online successfully */
	REQUIRE(!swclt_hmon_register(&hmon, sess, __on_sess_hmon_event, "bobo"));

	{
		ks_handle_t next = 0;
		uint32_t count = 0;

		while (KS_STATUS_SUCCESS == ks_handle_enum_type(SWCLT_HTYPE_HMON, &next))
			count++;

		REQUIRE(count == 1);
	}

	ks_cond_lock(g_cond);

	/* Now take the session onine */
	REQUIRE(!swclt_sess_connect(sess));

	ks_cond_wait(g_cond);

	/* Should be online */
	REQUIRE(g_last_state_change == SWCLT_HSTATE_ONLINE);

	/* Now disconnect the session */
	REQUIRE(!swclt_sess_disconnect(sess));

	/* Should be offline now */
	ks_cond_wait(g_cond);

	REQUIRE(g_last_state_change == SWCLT_HSTATE_OFFLINE);

	/* Now delete the monitor */
	ks_handle_destroy(&hmon);

	ks_cond_unlock(g_cond);

	/* Go online again */
	REQUIRE(!swclt_sess_connect(sess));

	/* Wait for its low level handle state to be back to OK */
	while (swclt_hstate_check(sess, NULL))
		ks_sleep(1000);

	/* Ok we're done */
	ks_cond_destroy(&g_cond);
	ks_handle_destroy(&sess);
}
