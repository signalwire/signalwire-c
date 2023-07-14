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

/* The method name for a broadcast request */
#define BLADE_BROADCAST_METHOD "blade.broadcast"

/* Flags for the command, in our case we don't get replies */
#define BLADE_BROADCAST_FLAGS SWCLT_CMD_FLAG_NOREPLY

/* Default time to live for broadcast */
#define BLADE_BROADCAST_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

/* Create our broadcast request template */
typedef struct blade_broadcast_rqu_s {
	const char *protocol;
	const char *channel;
	const char *event;
	const char *broadcaster_nodeid;
	ks_json_t *params;
} blade_broadcast_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_BROADCAST_RQU, blade_broadcast_rqu_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(channel)
	SWCLT_JSON_MARSHAL_STRING(event)
	SWCLT_JSON_MARSHAL_STRING(broadcaster_nodeid)
	SWCLT_JSON_MARSHAL_ITEM(params)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_BROADCAST_RQU, blade_broadcast_rqu_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(channel)
	SWCLT_JSON_DESTROY_STRING(event)
	SWCLT_JSON_DESTROY_STRING(broadcaster_nodeid)
	SWCLT_JSON_DESTROY_ITEM(params)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_BROADCAST_RQU, blade_broadcast_rqu_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(channel)
	SWCLT_JSON_PARSE_STRING(event)
	SWCLT_JSON_PARSE_STRING(broadcaster_nodeid)
	SWCLT_JSON_PARSE_ITEM(params)
SWCLT_JSON_PARSE_END()

/**
 * CREATE_BLADE_BROADCAST_CMD_ASYNC - Creates a command with a request
 * in it setup to submit a broadcast method to blade.
 */
static inline swclt_cmd_t *CREATE_BLADE_BROADCAST_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char *protocol,
	const char *channel,
	const char *event,
	const char *broadcast_nodeid,
	ks_json_t **params)
{
	ks_json_t *obj = NULL;
	blade_broadcast_rqu_t broadcast_rqu;
	swclt_cmd_t *cmd = NULL;

	/* Fill in the broadcast request then marshal it, it will create copies
	 * of all the fields so caller doesn't lose ownership here */
	broadcast_rqu.protocol = protocol;
	broadcast_rqu.channel = channel;
	broadcast_rqu.event = event;
	broadcast_rqu.broadcaster_nodeid = broadcast_nodeid;
	broadcast_rqu.params = (!params || !*params) ? ks_json_create_object() : *params;

	if (!(obj = BLADE_BROADCAST_RQU_MARSHAL(&broadcast_rqu))) {

		/* Since params is last, on error here we can be sure params was
		 * not freed so do not set the callers params to NULL */
		return cmd;
	}

	/* We now own the callers params, null it to indicate ownership transfer */
	if (params)
		*params = NULL;

	/* Now give it to the new command */
	if (swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_BROADCAST_METHOD,
			&obj,
			BLADE_BROADCAST_TTL_MS,
			BLADE_BROADCAST_FLAGS,
			ks_uuid_null()))
		goto done;

done:
	ks_json_delete(&obj);
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_BROADCAST_CMD(
	const char *protocol,
	const char *channel,
	const char *event,
	const char *broadcast_nodeid,
	ks_json_t **params)
{
	return CREATE_BLADE_BROADCAST_CMD_ASYNC(
		NULL,
		NULL,
		protocol,
		channel,
		event,
		broadcast_nodeid,
		params);
}
