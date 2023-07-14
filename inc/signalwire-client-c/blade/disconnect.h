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

/* The method name for a disconnect request */
#define BLADE_DISCONNECT_METHOD "blade.disconnect"

/* Flags for the command, in our case we don't get replies */
#define BLADE_DISCONNECT_FLAGS SWCLT_CMD_FLAG_NOREPLY

/* Default time to live for disconnect */
#define BLADE_DISCONNECT_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

/* Create our disconnect request template */
typedef struct blade_disconnect_rqu_s {
	int unused;
} blade_disconnect_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_DISCONNECT_RQU, blade_disconnect_rqu_t)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_DISCONNECT_RQU, blade_disconnect_rqu_t)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_DISCONNECT_RQU, blade_disconnect_rqu_t)
SWCLT_JSON_PARSE_END()

/**
 * CREATE_BLADE_DISCONNECT_CMD_ASYNC - Creates a command with a request
 * in it setup to submit a disconnect method to blade.
 */
static inline swclt_cmd_t *CREATE_BLADE_DISCONNECT_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data)
{
	ks_json_t *obj = NULL;
	blade_disconnect_rqu_t disconnect_rqu;
	swclt_cmd_t *cmd = NULL;

	/* Fill in the disconnect request then marshal it, it will create copies
	 * of all the fields so caller doesn't lose ownership here */

	if (!(obj = BLADE_DISCONNECT_RQU_MARSHAL(&disconnect_rqu))) {

		/* Since params is last, on error here we can be sure params was
		 * not freed so do not set the callers params to NULL */
		return cmd;
	}

	/* Now give it to the new command */
	if (swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_DISCONNECT_METHOD,
			&obj,
			BLADE_DISCONNECT_TTL_MS,
			BLADE_DISCONNECT_FLAGS,
			ks_uuid_null()))
		goto done;

done:
	ks_json_delete(&obj);
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_DISCONNECT_CMD(void)
{
	return CREATE_BLADE_DISCONNECT_CMD_ASYNC(
		NULL,
		NULL);
}
