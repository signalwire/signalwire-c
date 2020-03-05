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

static ks_cond_t *g_cond;

static void __on_sess_state_event(swclt_sess_t *sess, void *cb_data)
{
	REQUIRE(!strcmp((char *)cb_data, "bobo"));
	ks_cond_lock(g_cond);
	ks_cond_broadcast(g_cond);
	ks_cond_unlock(g_cond);
}

void test_uncert_exp(ks_pool_t *pool)
{
	swclt_sess_t *sess;

	REQUIRE(!ks_cond_create(&g_cond, NULL));

	/* Load the config we expect session to load */
	REQUIRE(swclt_config_get_authentication(g_uncertified_config));

	REQUIRE(!swclt_sess_create(&sess, g_target_ident_str, g_uncertified_config));

	/* Register a monitor to get to know when session comes online successfully */
	REQUIRE(!swclt_sess_set_state_change_cb(sess, __on_sess_state_event, "bobo"));

	ks_cond_lock(g_cond);

	/* Now take the session onine */
	REQUIRE(!swclt_sess_connect(sess));
	int i = 5;
	while (i-- > 0 && !swclt_sess_connected(sess)) {
		ks_cond_timedwait(g_cond, 1000);
	}
	REQUIRE(swclt_sess_connected(sess));

	/* Now disconnect the session */
	REQUIRE(!swclt_sess_disconnect(sess));
	i = 5;
	while (i-- > 0 && swclt_sess_connected(sess)) {
		ks_cond_timedwait(g_cond, 1000);
	}
	REQUIRE(!swclt_sess_connected(sess));

	/* Go online again */
	REQUIRE(!swclt_sess_connect(sess));
	i = 5;
	while (i-- > 0 && !swclt_sess_connected(sess)) {
		ks_cond_timedwait(g_cond, 1000);
	}
	REQUIRE(swclt_sess_connected(sess));

	ks_cond_unlock(g_cond);

	swclt_sess_destroy(&sess);

	/* Ok we're done */
	ks_cond_destroy(&g_cond);
}
