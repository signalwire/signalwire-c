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

#include "../../src/connection.c"

static uint32_t g_protocol_response_cb_called;

static ks_status_t __on_incoming_cmd(swclt_conn_t *conn, swclt_cmd_t *cmd, void *cb_data)
{
	printf("ON INCOMING COMMAND\n");
	return KS_STATUS_SUCCESS;
}

static void __on_protocol_timeout_response(swclt_cmd_reply_t *reply, void *cb_data)
{
	REQUIRE(reply);
	REQUIRE(reply->type == SWCLT_CMD_TYPE_FAILURE);
	REQUIRE(reply->failure_reason);
	REQUIRE(reply->failure_status == KS_STATUS_TIMEOUT);
	printf("Validated failure code, message: %s\n", reply->failure_reason);
	swclt_cmd_reply_destroy(&reply);
	g_protocol_response_cb_called++;
}

static void __on_protocol_result_response(swclt_cmd_reply_t *reply, void *cb_data)
{
	REQUIRE(swclt_cmd_reply_ok(reply) == KS_STATUS_SUCCESS);
	swclt_cmd_reply_destroy(&reply);
	g_protocol_response_cb_called++;
}

void test_async(ks_pool_t *pool)
{
	swclt_cmd_t *cmd;
	SSL_CTX *ssl = create_ssl_context();
	swclt_conn_t *conn;
	ks_json_t *channels;
	int i;

	REQUIRE(!swclt_conn_connect(&conn, __on_incoming_cmd, NULL, &g_target_ident, NULL, NULL, NULL, NULL, ssl));

	channels = ks_json_create_array();
	ks_json_add_item_to_array(channels, BLADE_CHANNEL_MARSHAL(&(blade_channel_t){"a_channel", 0, 0}));

	/* Create an async command (bogus command but will generate a reply at least) */
	REQUIRE(cmd = CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD_ASYNC(
			__on_protocol_result_response,
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
	REQUIRE(!swclt_conn_submit_request(conn, &cmd, NULL));

	/* Wait for it to respond */
	for (i = 0; i < 5 && g_protocol_response_cb_called == 0; i++) {
		ks_sleep_ms(1000);
	}
	REQUIRE(g_protocol_response_cb_called == 1);
	swclt_cmd_destroy(&cmd);
	swclt_conn_destroy(&conn);
	swclt_ssl_destroy_context(&ssl);
}

void test_ttl(ks_pool_t *pool)
{
	SSL_CTX *ssl = create_ssl_context();
	swclt_conn_t *conn;
	swclt_cmd_t *cmd;
	SWCLT_CMD_TYPE cmd_type;
	ks_json_t *channels;
	int i;

	g_protocol_response_cb_called = 0;

	REQUIRE(!swclt_conn_connect(&conn, __on_incoming_cmd, NULL, &g_target_ident, NULL, NULL, NULL, NULL, ssl));

	channels = ks_json_create_array();
	ks_json_add_item_to_array(channels, BLADE_CHANNEL_MARSHAL(&(blade_channel_t){"b_channel", 0, 0}));
	REQUIRE(cmd = CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD_ASYNC(
			__on_protocol_timeout_response,
			NULL,
			"b_protocol",
			0,
			0,
			0,
			NULL,
		    &channels,
			1,
			NULL));

	/* Lock the reader so we never get a response, forcing a timeout */
	REQUIRE(cmd->response_ttl_ms == BLADE_PROTOCOL_TTL_MS);
	REQUIRE(cmd->flags == BLADE_PROTOCOL_FLAGS);
	REQUIRE(!ks_mutex_lock(conn->wss->wss_mutex));

	/* And submit it */
	REQUIRE(!swclt_conn_submit_request(conn, &cmd, NULL));

	/* Wait for it to respond */
	for (i = 0; i < 7 && g_protocol_response_cb_called == 0; i++) {
		ks_sleep_ms(1000);
	}

	REQUIRE(g_protocol_response_cb_called == 1);

	/* Don't forget to unlock the poor websocket reader */
	REQUIRE(!ks_mutex_unlock(conn->wss->wss_mutex));

	swclt_cmd_destroy(&cmd);
	swclt_conn_destroy(&conn);
	swclt_ssl_destroy_context(&ssl);
}

void test_ttl_heap(ks_pool_t *pool)
{
	swclt_ttl_tracker_t *ttl = NULL;
	ttl_tracker_create(pool, &ttl, NULL);
	if (ttl->thread) {
		ks_thread_request_stop(ttl->thread);
		ks_thread_join(ttl->thread);
		ks_thread_destroy(&ttl->thread); // don't want it
	}

	int i;
	int min = INT_MAX;
	for (i = 0; i < 100; i++) {
		ks_uuid_t uuid = { 0 };
		ks_uuid(&uuid);
		int expiry = rand();
		if (expiry < min) {
			min = expiry;
		}
		if (ttl_heap_insert(ttl, expiry, uuid) != KS_STATUS_SUCCESS) {
			printf("Failed to insert UUID = %s, TTL = %d, min = %d, root = %d, count = %d\n", ks_uuid_thr_str(&uuid), expiry, min, ttl->heap[TTL_HEAP_ROOT].expiry, ttl->count);
		} else {
			printf("Insert UUID = %s, TTL = %d, min = %d, root = %d, count = %d\n", ks_uuid_thr_str(&uuid), expiry, min, ttl->heap[TTL_HEAP_ROOT].expiry, ttl->count);
		}
		REQUIRE(ttl->heap[TTL_HEAP_ROOT].expiry == min);
	}

	min = 0;
	while (ttl->count) {
		REQUIRE(ttl->heap[TTL_HEAP_ROOT].expiry > 0);
		printf("Remove UUID = %s, TTL = %d, count = %d\n", ks_uuid_thr_str(&ttl->heap[TTL_HEAP_ROOT].id), ttl->heap[TTL_HEAP_ROOT].expiry, ttl->count);
		REQUIRE(ttl->heap[TTL_HEAP_ROOT].expiry >= min);
		min = ttl->heap[TTL_HEAP_ROOT].expiry;
		ttl_heap_remove(ttl);
	}

	ttl_tracker_destroy(&ttl);
}

void test_connection(ks_pool_t *pool)
{
	test_async(pool);
	test_ttl(pool);
	test_ttl_heap(pool);
}
