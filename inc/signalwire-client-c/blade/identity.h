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

/* The method name for an identity request */
#define BLADE_IDENTITY_METHOD "blade.identity"

/* Flags for the command */
#define BLADE_IDENTITY_FLAGS 0

/* Default time to live for identity */
#define BLADE_IDENTITY_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

#define BLADE_IDENTITY_CMD_ADD		"add"
#define BLADE_IDENTITY_CMD_REMOVE	"remove"

typedef struct blade_identity_rqu_s {
	const char *command;
	ks_json_t *identities;
} blade_identity_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_IDENTITY_RQU, blade_identity_rqu_t)
	SWCLT_JSON_MARSHAL_STRING(command)
	SWCLT_JSON_MARSHAL_ITEM(identities)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_IDENTITY_RQU, blade_identity_rqu_t)
	SWCLT_JSON_DESTROY_STRING(command)
	SWCLT_JSON_DESTROY_ITEM(identities)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_IDENTITY_RQU, blade_identity_rqu_t)
	SWCLT_JSON_PARSE_STRING(command)
	SWCLT_JSON_PARSE_ITEM(identities)
SWCLT_JSON_PARSE_END()

/**
 * CREATE_BLADE_IDENTITY_CMD_ASYNC - Creates a command which holds
 * and owns the request json for an identity request.
 */
static inline swclt_cmd_t *CREATE_BLADE_IDENTITY_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char *command,
	const char *identity)
{
	swclt_cmd_t *cmd = NULL;
	ks_status_t status;
	ks_json_t *identities;
	ks_json_t *request;

	/* Serialize the identity into an array */
	if (!(identities = ks_json_create_array())) {
		ks_log(KS_LOG_WARNING, "Failed to allocate blade identity array");
		return cmd;
	}
	ks_json_add_string_to_array(identities, identity);

	if (!(request = BLADE_IDENTITY_RQU_MARSHAL(
			&(blade_identity_rqu_t){
				command,
				identities}))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate identity request");
		return cmd;
	}

	/* Now hand it to the command, it will take ownership of it if successful
	 * and null out our ptr */
	if ((status = swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_IDENTITY_METHOD,
			&request,
			BLADE_IDENTITY_TTL_MS,
			BLADE_IDENTITY_FLAGS,
			ks_uuid_null()))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate identity cmd: %lu", status);

		/* Safe to free this or at least attempt to, cmd will have set it to null if it
		 * took ownership of it */
		ks_json_delete(&request);
		return NULL;
	}

	/* Phew, successfully allocated the command */
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_IDENTITY_CMD(
	const char *command,
	const char *identity)
{
	return CREATE_BLADE_IDENTITY_CMD_ASYNC(
		NULL,
		NULL,
		command,
		identity);
}
