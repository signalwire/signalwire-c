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

void test_command_properties(ks_pool_t *pool)
{
	swclt_cmd_t cmd;
	uint32_t flags;
	const char *method;
	SWCLT_CMD_TYPE type;
	ks_json_t *request;

	request = ks_json_create_object();
	ks_json_add_string_to_object(request, "bobo_key", "bobo_val");
	REQUIRE(request != NULL);

	REQUIRE(!swclt_cmd_create(&cmd, "bobo_method", &request, 5000, 5));

	REQUIRE(!swclt_cmd_flags(cmd, &flags));
	REQUIRE(flags == 1);	/* bitfields will imit it to known values */

	REQUIRE(!swclt_cmd_method(cmd, &method));
	REQUIRE(!strcmp(method, "bobo_method"));

	REQUIRE(!swclt_cmd_type(cmd, &type));
	REQUIRE(type == SWCLT_CMD_TYPE_REQUEST);
	ks_handle_destroy(&cmd);
}

void test_command(ks_pool_t *pool)
{
	test_command_properties(pool);
}
