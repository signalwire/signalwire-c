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

#pragma once

/* The method name for a protocol request */
#define BLADE_EXECUTE_METHOD "blade.execute"

/* Flags for the command */
#define BLADE_EXECUTE_FLAGS 0

/* Default time to live for protocol */
#define BLADE_EXECUTE_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

typedef struct blade_execute_m_rqu_s {
	const char *requester_nodeid;
	const char *responder_nodeid;
	const char *protocol;
	const char *method;
	ks_json_t **params;
} blade_execute_m_rqu_t;

typedef struct blade_execute_rqu_s {
	const char *requester_nodeid;
	const char *responder_nodeid;
	const char *protocol;
	const char *method;
	ks_json_t *params;
} blade_execute_rqu_t;

SWCLT_JSON_DESTROY_BEG(BLADE_EXECUTE_RQU, blade_execute_rqu_t)
	SWCLT_JSON_DESTROY_STRING(requester_nodeid)
	SWCLT_JSON_DESTROY_STRING(responder_nodeid)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(method)
	SWCLT_JSON_DESTROY_ITEM(params)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_EXECUTE_RQU, blade_execute_rqu_t)
	SWCLT_JSON_PARSE_STRING(requester_nodeid)
	SWCLT_JSON_PARSE_STRING(responder_nodeid)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(method)
	SWCLT_JSON_PARSE_ITEM(params)
SWCLT_JSON_PARSE_END()

typedef struct blade_execute_rpl_s {
	const char *requester_nodeid;
	const char *responder_nodeid;
	ks_json_t *result;
} blade_execute_rpl_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_EXECUTE_RPL, blade_execute_rpl_t)
	SWCLT_JSON_MARSHAL_STRING(requester_nodeid)
	SWCLT_JSON_MARSHAL_STRING(responder_nodeid)
	SWCLT_JSON_MARSHAL_ITEM(result)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_EXECUTE_RPL, blade_execute_rpl_t)
	SWCLT_JSON_DESTROY_STRING(requester_nodeid)
	SWCLT_JSON_DESTROY_STRING(responder_nodeid)
	SWCLT_JSON_DESTROY_ITEM(result)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_EXECUTE_RPL, blade_execute_rpl_t)
	SWCLT_JSON_PARSE_STRING(requester_nodeid)
	SWCLT_JSON_PARSE_STRING(responder_nodeid)
	SWCLT_JSON_PARSE_ITEM(result)
SWCLT_JSON_PARSE_END()

typedef struct blade_execute_err_s {
	const char *requester_nodeid;
	const char *responder_nodeid;
	int code;
	const char *message;
} blade_execute_err_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_EXECUTE_ERR, blade_execute_err_t)
	SWCLT_JSON_MARSHAL_STRING(requester_nodeid)
	SWCLT_JSON_MARSHAL_STRING(responder_nodeid)
	SWCLT_JSON_MARSHAL_INT(code)
	SWCLT_JSON_MARSHAL_STRING(message)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_EXECUTE_ERR, blade_execute_err_t)
	SWCLT_JSON_DESTROY_STRING(requester_nodeid)
	SWCLT_JSON_DESTROY_STRING(responder_nodeid)
	SWCLT_JSON_DESTROY_INT(code)
	SWCLT_JSON_DESTROY_STRING(message)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_EXECUTE_ERR, blade_execute_err_t)
	SWCLT_JSON_PARSE_STRING(requester_nodeid)
	SWCLT_JSON_PARSE_STRING(responder_nodeid)
	SWCLT_JSON_PARSE_INT(code)
	SWCLT_JSON_PARSE_STRING(message)
SWCLT_JSON_PARSE_END()

/**
 * CREATE_BLADE_EXECUTE_CMD_ASYNC - Creates a command which holds
 * and owns the request json for an execute request.
 */
static inline swclt_cmd_t *CREATE_BLADE_EXECUTE_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char *id,
	const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params)
{
	swclt_cmd_t *cmd = NULL;
	ks_status_t status;
	ks_json_t *request;
	ks_uuid_t msgid = ks_uuid_null();

	if (id) msgid = ks_uuid_from_str(id);
	request = ks_json_create_object();
	if (responder) ks_json_add_string_to_object(request, "responder_nodeid", responder);
	ks_json_add_string_to_object(request, "protocol", protocol);
	ks_json_add_string_to_object(request, "method", method);
	ks_json_add_item_to_object(request, "params", *params);

	/* Clear callers ptr */
	*params = NULL;

	/* Now hand it to the command, it will take ownership of it if successful
	 * and null out our ptr */
	if ((status = swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_EXECUTE_METHOD,
			&request,
			BLADE_EXECUTE_TTL_MS,
			BLADE_EXECUTE_FLAGS,
			msgid))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate execute cmd: %lu", status);

		/* Safe to free this or at least attempt to, cmd will have set it to null if it
		 * took ownership of it */
		ks_json_delete(&request);
		return NULL;
	}

	/* Phew, successfully allocated the command */
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_EXECUTE_CMD(
	const char *id,
    const char *responder,
	const char *protocol,
	const char *method,
	ks_json_t **params)
{
	return CREATE_BLADE_EXECUTE_CMD_ASYNC(
		NULL,
		NULL,
		id,
		responder,
		protocol,
		method,
		params);
}


