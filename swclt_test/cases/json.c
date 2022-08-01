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

static void test_blade_execute(ks_pool_t *pool)
{
	ks_json_t *params = ks_json_create_object();
	ks_json_add_string_to_object(params, "tag", "hi");
	swclt_cmd_t *cmd = CREATE_BLADE_EXECUTE_CMD(
			NULL,
		    "responder_a",
		   	"test.protocol",
		   	"test.method",
			&params);
	REQUIRE(!params);

	char *desc;
	REQUIRE(!swclt_cmd_print(cmd, NULL, &desc));
	ks_log(KS_LOG_INFO, "Created command: %s", desc);

	REQUIRE(!strcmp(ks_json_get_object_string(cmd->json, "responder_nodeid", ""), "responder_a"));
	REQUIRE(!strcmp(ks_json_get_object_string(cmd->json, "protocol", ""), "test.protocol"));
	REQUIRE(!strcmp(ks_json_get_object_string(cmd->json, "method", ""), "test.method"));
	REQUIRE(!strcmp(ks_json_get_object_string(ks_json_get_object_item(cmd->json, "params"), "tag", ""), "hi"));

	ks_pool_free(&desc);
	swclt_cmd_destroy(&cmd);
}

static void test_blade_execute_with_id(ks_pool_t *pool)
{
	ks_json_t *params = ks_json_create_object();
	ks_json_add_string_to_object(params, "tag", "hi");
	swclt_cmd_t *cmd = CREATE_BLADE_EXECUTE_CMD(
			"a6786101-4c6e-4d1a-ac22-1c5dcbbd3c48",
		    "responder_a",
		   	"test.protocol",
		   	"test.method",
			&params);
	REQUIRE(!params);

	char *desc;
	REQUIRE(!swclt_cmd_print(cmd, NULL, &desc));
	ks_log(KS_LOG_INFO, "Created command: %s", desc);

	REQUIRE(!strcmp(cmd->id_str, "a6786101-4c6e-4d1a-ac22-1c5dcbbd3c48"));
	REQUIRE(!strcmp(ks_json_get_object_string(cmd->json, "responder_nodeid", ""), "responder_a"));
	REQUIRE(!strcmp(ks_json_get_object_string(cmd->json, "protocol", ""), "test.protocol"));
	REQUIRE(!strcmp(ks_json_get_object_string(cmd->json, "method", ""), "test.method"));
	REQUIRE(!strcmp(ks_json_get_object_string(ks_json_get_object_item(cmd->json, "params"), "tag", ""), "hi"));

	ks_pool_free(&desc);
	swclt_cmd_destroy(&cmd);
}

static void test_blade_channel(ks_pool_t *pool)
{
	ks_json_t *obj = BLADE_CHANNEL_MARSHAL(&(blade_channel_t){"bobo", BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC});
	ks_json_delete(&obj);
}

void test_json(ks_pool_t *pool)
{
	test_blade_channel(pool);
	test_blade_execute(pool);
	test_blade_execute_with_id(pool);
}
