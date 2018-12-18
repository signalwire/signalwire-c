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

static ks_bool_t g_called = KS_FALSE;

void route_add_handler(swclt_sess_t sess, const blade_netcast_rqu_t *rqu, const blade_netcast_route_add_param_t *params)
{
	g_called = KS_TRUE;
}

void test_callback(ks_pool_t *pool)
{
	swclt_sess_t sess;
	swclt_sess_ctx_t *sess_ctx;
	swclt_store_t store;

	/* Load the config we expect session to load */
	REQUIRE(swclt_config_get_private_key_path(g_certified_config));
	REQUIRE(swclt_config_get_client_cert_path(g_certified_config));
	REQUIRE(swclt_config_get_cert_chain_path(g_certified_config));

	REQUIRE(!swclt_sess_create(&sess, g_target_ident_str, g_certified_config));

	/* Get the node store */
	REQUIRE(!swclt_sess_nodestore(sess, &store));

	/* Add a store callback */
	REQUIRE(!swclt_store_cb_route_add(store, route_add_handler));

	/* Go online */
	REQUIRE(!swclt_sess_connect(sess));

	/* Wait for its low level handle state to be ready */
	while (!swclt_sess_connected(sess))
		ks_sleep(1000);

	ks_log(KS_LOG_INFO, "*** Online!");

	/* Wait for the netcast */
	ks_sleep_ms(5000);

	REQUIRE(g_called);

	/* Ok we're done */
	ks_handle_destroy(&sess);
}
