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

#include "swclt_test.h"

static uint32_t g_protocol_response_cb_called;

static ks_status_t __on_incoming_cmd(swclt_conn_t conn, swclt_cmd_t cmd, void *cb_data)
{
	return KS_STATUS_SUCCESS;
}

static void __on_protocol_response(swclt_cmd_t cmd, void *cb_data)
{
	g_protocol_response_cb_called++;
}

void test_async(ks_pool_t *pool)
{
	SSL_CTX *ssl = create_ssl_context();
	swclt_conn_t conn;
	swclt_cmd_t cmd;
	SWCLT_CMD_TYPE cmd_type;
	const ks_json_t *result;
	ks_json_t *channels;

	REQUIRE(!swclt_conn_connect(&conn, __on_incoming_cmd, NULL, &g_target_ident, NULL, ssl));

	channels = ks_json_create_array_inline(1, BLADE_CHANNEL_MARSHAL(pool, &(blade_channel_t){"a_channel", 0, 0}));

	/* Create an async command (bogus command but will generate a reply at least) */
	REQUIRE(cmd = CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD_ASYNC(
			__on_protocol_response,
			NULL,
			"a_protocol",
			0,
			0,
			0,
			NULL,
			&channels,
			1,
			NULL));

	/* And submit it */
	REQUIRE(!swclt_conn_submit_request(conn, cmd));

	/* Wait for it to respond */
	while (KS_TRUE) {

		REQUIRE(!swclt_cmd_type(cmd, &cmd_type));

		if (cmd_type != SWCLT_CMD_TYPE_REQUEST) {
			break;
		}

		ks_sleep_ms(1000);
	}

	REQUIRE(cmd_type == SWCLT_CMD_TYPE_RESULT);
	REQUIRE(!swclt_cmd_result(cmd, &result));
	REQUIRE(g_protocol_response_cb_called == 1);

	REQUIRE(ks_handle_valid(cmd));

	ks_handle_destroy(&conn);

	/* Command should become invalid once we destroy the connection */
	REQUIRE(!ks_handle_valid(cmd));
	swclt_ssl_destroy_context(&ssl);
}

void test_ttl(ks_pool_t *pool)
{
	SSL_CTX *ssl = create_ssl_context();
	swclt_conn_t conn;
	swclt_cmd_t cmd;
	SWCLT_CMD_TYPE cmd_type;
	swclt_conn_ctx_t *conn_ctx;
	swclt_wss_ctx_t *wss_ctx;
	swclt_cmd_ctx_t *cmd_ctx;
	ks_json_t *channels;

	REQUIRE(!swclt_conn_connect(&conn, __on_incoming_cmd, NULL, &g_target_ident, NULL, ssl));

	channels = ks_json_create_array_inline(1, BLADE_CHANNEL_MARSHAL(pool, &(blade_channel_t){"a_channel", 0, 0}));
	REQUIRE(cmd = CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD_ASYNC(
			__on_protocol_response,
			NULL,
			"a_protocol",
			0,
			0,
			0,
			NULL,
		    &channels,
			1,
			NULL));

	/* Lock the reader so we never get a response, forcing a timeout */
	conn_ctx = conn_get(conn);
	wss_ctx = wss_get(conn_ctx->wss);
	cmd_ctx = cmd_get(cmd);
	REQUIRE(cmd_ctx->response_ttl_ms == BLADE_PROTOCOL_TTL_MS);
	REQUIRE(cmd_ctx->flags == BLADE_PROTOCOL_FLAGS);
	REQUIRE(!ks_mutex_lock(wss_ctx->write_mutex));

	/* And submit it */
	REQUIRE(!swclt_conn_submit_request(conn, cmd));

	/* Give it 4 seconds */
	ks_sleep_ms(4000);

	{
		ks_status_t failure_status;
		const char *failure_message;

		/* Better have failed */
		REQUIRE(!swclt_cmd_type(cmd, &cmd_type));

		REQUIRE(cmd_type == SWCLT_CMD_TYPE_FAILURE);
		REQUIRE(!swclt_cmd_failure_info(cmd, &failure_status, &failure_message));

		/* One for this test, and the one prior */
		REQUIRE(g_protocol_response_cb_called == 2);

		REQUIRE(failure_status == KS_STATUS_TIMEOUT);
		printf("Validated failure code, message: %s\n", failure_message);
	}

	/* Don't forget to unlock the poor websocket reader */
	REQUIRE(!ks_mutex_unlock(wss_ctx->write_mutex));

	cmd_put(&cmd_ctx);
	conn_put(&conn_ctx);
	wss_put(&wss_ctx);

	REQUIRE(ks_handle_valid(cmd));

	ks_handle_destroy(&conn);

	/* Command should become invalid once we destroy the connection */
	REQUIRE(!ks_handle_valid(cmd));
	swclt_ssl_destroy_context(&ssl);
}

void test_connection(ks_pool_t *pool)
{
	test_async(pool);
	test_ttl(pool);
}
