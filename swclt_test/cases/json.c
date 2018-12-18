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

static blade_netcast_rqu_t __netcast_protocol_provider_add_request(ks_pool_t *pool, const char *protocol, ks_uuid_t nodeid, const char *channel)
{
	blade_netcast_rqu_t request;
	request.command = BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD;
	request.certified_only = KS_TRUE;
	request.netcaster_nodeid = ks_uuid_null_thr_str();

	/* Fill in the params too */
	blade_netcast_protocol_provider_add_param_t params;
	params.protocol = protocol;
	params.nodeid = ks_uuid_str(NULL, &nodeid);
	params.channels = ks_json_create_array_inline(1,
		BLADE_CHANNEL_MARSHAL(NULL, &(blade_channel_t){channel, BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC}));

	/* Marshal it into its parent request */
	request.params = BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_MARSHAL(NULL, &params);

	return request;
}

static void test_blade_execute(ks_pool_t *pool)
{
	ks_json_t *params = ks_json_create_object();
	ks_json_padd_string_to_object(NULL, params, "tag", "hi");
	swclt_cmd_t cmd = CREATE_BLADE_EXECUTE_CMD(
		    "responder_a",
		   	"test.protocol",
		   	"test.method",
			&params);
	REQUIRE(!params);

	const ks_json_t *obj;
	REQUIRE(swclt_cmd_error(cmd, &obj));
	REQUIRE(swclt_cmd_result(cmd, &obj));
	REQUIRE(!swclt_cmd_request(cmd, &obj));

	char *desc;
	REQUIRE(!swclt_cmd_print(cmd, NULL, &desc));
	ks_log(KS_LOG_INFO, "Created command: %s", desc);

	REQUIRE(!strcmp(ks_json_get_object_cstr(obj, "responder_nodeid"), "responder_a"));
	REQUIRE(!strcmp(ks_json_get_object_cstr(obj, "protocol"), "test.protocol"));
	REQUIRE(!strcmp(ks_json_get_object_cstr(obj, "method"), "test.method"));
	REQUIRE(!strcmp(ks_json_lookup_cstr(obj, 2, "params", "tag"), "hi"));

	ks_pool_free(&desc);
	ks_handle_destroy(&cmd);
}

static void test_blade_channel(ks_pool_t *pool)
{
	ks_json_t *obj = BLADE_CHANNEL_MARSHAL(NULL, &(blade_channel_t){"bobo", BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC});
	ks_json_delete(&obj);
}

void test_json(ks_pool_t *pool)
{
	test_blade_channel(pool);
	test_blade_execute(pool);
}
