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

#pragma once

KS_BEGIN_EXTERN_C

static const char *BLADE_SUBSCRIPTION_CMD_ADD = "add";
static const char *BLADE_SUBSCRIPTION_CMD_REMOVE = "remove";

/* The method name for a subscription request */
static const char *BLADE_SUBSCRIPTION_METHOD = "blade.subscription";

/* Flags for the command */
#define BLADE_SUBSCRIPTION_FLAGS 0

/* Default ttl for subscription cmd */
#define BLADE_SUBSCRIPTION_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

typedef struct blade_subscription_rpl_s {
	const char *command;
	const char *protocol;
	ks_json_t *failed_channels;
	ks_json_t *subscribe_channels;
} blade_subscription_rpl_t;

SWCLT_JSON_DESTROY_BEG(BLADE_SUBSCRIPTION_RPL, blade_subscription_rpl_t)
	SWCLT_JSON_DESTROY_STRING(command)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_ITEM(failed_channels)
	SWCLT_JSON_DESTROY_ITEM(subscribe_channels)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_SUBSCRIPTION_RPL, blade_subscription_rpl_t)
	SWCLT_JSON_PARSE_STRING(command)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_ITEM_OPT(failed_channels)
	SWCLT_JSON_PARSE_ITEM_OPT(subscribe_channels)
SWCLT_JSON_PARSE_END()

typedef struct blade_subscription_rqu_s {
	const char *command;
	const char *protocol;
	ks_json_t *channels;
} blade_subscription_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_SUBSCRIPTION_RQU, blade_subscription_rqu_t)
	SWCLT_JSON_MARSHAL_STRING(command)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_ITEM(channels)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_SUBSCRIPTION_RQU, blade_subscription_rqu_t)
	SWCLT_JSON_DESTROY_STRING(command)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_ITEM(channels)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_SUBSCRIPTION_RQU, blade_subscription_rqu_t)
	SWCLT_JSON_PARSE_STRING(command)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_ITEM(channels)
SWCLT_JSON_PARSE_END()

/**
 * CREATE_BLADE_SUBSCRIPTION_CMD_ASYNC - Creates a command which holds
 * and owns the request json for a subscription request.
 */
static inline swclt_cmd_t CREATE_BLADE_SUBSCRIPTION_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char *command,
	const char *protocol,
	const char *channel)
{
	ks_pool_t *pool;
	ks_json_t *request_obj;
	ks_status_t status;
	swclt_cmd_t cmd = KS_NULL_HANDLE;

	if (ks_pool_open(&pool))
		return cmd;

	/* Create a blade subscription request structure then marshal it */
	blade_subscription_rqu_t request = {
		command,
		protocol,
		ks_json_pcreate_array_inline(pool, 1, ks_json_pcreate_string(pool, channel))
	};

	if (!(request_obj = BLADE_SUBSCRIPTION_RQU_MARSHAL(pool, &request))) {
		ks_log(KS_LOG_WARNING, "Failed to create subscription request");

		/* Don't forget to free the channels we instantiated inline above */
		ks_json_delete(&request.channels);
		ks_pool_close(&pool);
		return cmd;
	}

	/* Request object has been created, channels will now reside in it */

	/* Now wrap it in a command */
	if ((status = swclt_cmd_create_ex(
			&cmd,
			&pool,
			cb,
			cb_data,
			BLADE_SUBSCRIPTION_METHOD,
			&request_obj,
			BLADE_SUBSCRIPTION_FLAGS,
			BLADE_SUBSCRIPTION_TTL_MS,
			ks_uuid_null()))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate subscription command: %lu", status);
		ks_json_delete(&request_obj);
		ks_pool_close(&pool);
		return status;
	}

	/* Success */
	return cmd;
}

static inline swclt_cmd_t CREATE_BLADE_SUBSCRIPTION_CMD(
	const char *command,
	const char *protocol,
	const char *channel)
{
	return CREATE_BLADE_SUBSCRIPTION_CMD_ASYNC(
		NULL,
		NULL,
		command,
		protocol,
		channel);
}

/* Creates a subscription request */
static inline ks_json_t *BLADE_SUBSCRIPTION_RQU(
	ks_pool_t *pool,
	const char * const command,
	const char * const protocol,
	const char * const channel,
	blade_access_control_t broadcast_access,
	blade_access_control_t subscribe_access)
{
	ks_json_t *request = ks_json_pcreate_object(pool);
	ks_json_t *channels = ks_json_pcreate_array(pool);
	ks_json_padd_string_to_array(pool, channels, channel);
	ks_json_padd_string_to_object(pool, request, "command", command);
	ks_json_padd_string_to_object(pool, request, "protocol", protocol);
	ks_json_add_string_to_array(channels, channel);
	ks_json_add_item_to_object(request, "channels", channels);
	return request;
}

KS_END_EXTERN_C
