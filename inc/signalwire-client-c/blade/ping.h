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
#pragma once

/* The method name for a ping request */
#define BLADE_PING_METHOD "blade.ping"

/* Flags for the command, in our case we don't get replies */
#define BLADE_PING_FLAGS SWCLT_CMD_FLAG_NOREPLY

/* Default time to live for ping */
#define BLADE_PING_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

/* Create our ping request template */
typedef struct blade_ping_rqu_s {
	double timestamp;
	const char *payload;
} blade_ping_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_PING_RQU, blade_ping_rqu_t)
    SWCLT_JSON_MARSHAL_DOUBLE(timestamp)
    SWCLT_JSON_MARSHAL_STRING_OPT(payload)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_PING_RQU, blade_ping_rqu_t)
    SWCLT_JSON_DESTROY_DOUBLE(timestamp)
    SWCLT_JSON_DESTROY_STRING(payload)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_PING_RQU, blade_ping_rqu_t)
    SWCLT_JSON_PARSE_DOUBLE_OPT(timestamp)
    SWCLT_JSON_PARSE_STRING_OPT(payload)
SWCLT_JSON_PARSE_END()

typedef struct blade_ping_rpl_s {
	double timestamp;
	const char *payload;
} blade_ping_rpl_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_PING_RPL, blade_ping_rpl_t)
    SWCLT_JSON_MARSHAL_DOUBLE(timestamp)
    SWCLT_JSON_MARSHAL_STRING_OPT(payload)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_PING_RPL, blade_ping_rpl_t)
    SWCLT_JSON_DESTROY_DOUBLE(timestamp)
    SWCLT_JSON_DESTROY_STRING(payload)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_PING_RPL, blade_ping_rpl_t)
    SWCLT_JSON_PARSE_DOUBLE_OPT(timestamp)
    SWCLT_JSON_PARSE_STRING_OPT(payload)
SWCLT_JSON_PARSE_END()

/**
 * CREATE_BLADE_PING_CMD_ASYNC - Creates a command with a request
 * in it setup to submit a ping method to blade.
 */
static inline swclt_cmd_t *CREATE_BLADE_PING_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	double timestamp,
	const char *payload)
{
	ks_json_t *obj = NULL;
	swclt_cmd_t *cmd = NULL;


	/* Fill in the ping request then marshal it, it will create copies
	 * of all the fields so caller doesn't lose ownership here */

	if (!(obj = BLADE_PING_RQU_MARSHAL(&(blade_ping_rqu_t){
						timestamp,
						payload}))) {

		/* Since params is last, on error here we can be sure params was
		 * not freed so do not set the callers params to NULL */
		return cmd;
	}

	/* Now give it to the new command */
	if (swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_PING_METHOD,
			&obj,
			BLADE_PING_TTL_MS,
			BLADE_PING_FLAGS,
			ks_uuid_null()))
		goto done;

done:
	ks_json_delete(&obj);
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_PING_CMD(double timestamp, const char *payload)
{
	return CREATE_BLADE_PING_CMD_ASYNC(
		NULL,
		NULL,
		timestamp,
		payload);
}
