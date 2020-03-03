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

/* The method name for a protocol request */
#define BLADE_PROTOCOL_METHOD "blade.protocol"

/* Flags for the command */
#define BLADE_PROTOCOL_FLAGS 0

/* Default time to live for protocol */
#define BLADE_PROTOCOL_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

#define BLADE_PROTOCOL_CMD_PROVIDER_ADD "provider.add"
#define BLADE_PROTOCOL_CMD_PROVIDER_REMOVE "provider.remove"
#define BLADE_PROTOCOL_CMD_PROVIDER_RANK_UPDATE "provider.rank.update"
#define BLADE_PROTOCOL_CMD_CHANNEL_ADD "channel.add"
#define BLADE_PROTOCOL_CMD_CHANNEL_REMOVE "channel.remove"

typedef struct blade_protocol_rqu_s {
	const char *command;
	const char *protocol;
	ks_json_t *params;
} blade_protocol_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_PROTOCOL_RQU, blade_protocol_rqu_t)
	SWCLT_JSON_MARSHAL_STRING(command)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_ITEM_OPT(params)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_PROTOCOL_RQU, blade_protocol_rqu_t)
	SWCLT_JSON_DESTROY_STRING(command)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_ITEM(params)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_PROTOCOL_RQU, blade_protocol_rqu_t)
	SWCLT_JSON_PARSE_STRING(command)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_ITEM_OPT(params)
SWCLT_JSON_PARSE_END()


/* The params definition for BLADE_PROTOCOL_CMD_PROVIDER_ADD */
typedef struct blade_protocol_provider_add_param_s {
	blade_access_control_t default_method_execute_access;
	blade_access_control_t default_channel_subscribe_access;
	blade_access_control_t default_channel_broadcast_access;
	ks_json_t *methods;
	ks_json_t *channels;
	int rank;
	ks_json_t *data;
} blade_protocol_provider_add_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_PROTOCOL_PROVIDER_ADD_PARAM, blade_protocol_provider_add_param_t)
	SWCLT_JSON_MARSHAL_INT(default_method_execute_access)
	SWCLT_JSON_MARSHAL_INT(default_channel_subscribe_access)
	SWCLT_JSON_MARSHAL_INT(default_channel_broadcast_access)
	SWCLT_JSON_MARSHAL_ITEM_OPT(methods)
	SWCLT_JSON_MARSHAL_ITEM_OPT(channels)
	SWCLT_JSON_MARSHAL_INT(rank)
	SWCLT_JSON_MARSHAL_ITEM_OPT(data)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_PROTOCOL_PROVIDER_ADD_PARAM, blade_protocol_provider_add_param_t)
	SWCLT_JSON_DESTROY_INT(default_method_execute_access)
	SWCLT_JSON_DESTROY_INT(default_channel_subscribe_access)
	SWCLT_JSON_DESTROY_INT(default_channel_broadcast_access)
	SWCLT_JSON_DESTROY_ITEM(methods)
	SWCLT_JSON_DESTROY_ITEM(channels)
	SWCLT_JSON_DESTROY_INT(rank)
	SWCLT_JSON_DESTROY_ITEM(data)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_PROTOCOL_PROVIDER_ADD_PARAM, blade_protocol_provider_add_param_t)
	SWCLT_JSON_PARSE_INT_OPT_DEF(default_method_execute_access, BLADE_ACL_SYSTEM)
	SWCLT_JSON_PARSE_INT_OPT_DEF(default_channel_subscribe_access, BLADE_ACL_SYSTEM)
	SWCLT_JSON_PARSE_INT_OPT_DEF(default_channel_broadcast_access, BLADE_ACL_SYSTEM)
	SWCLT_JSON_PARSE_ITEM_OPT(methods)
	SWCLT_JSON_PARSE_ITEM_OPT(channels)
	SWCLT_JSON_PARSE_INT_OPT_DEF(rank, 1)
	SWCLT_JSON_PARSE_ITEM_OPT(data)
SWCLT_JSON_PARSE_END()


/* The params definition for BLADE_PROTOCOL_CMD_PROVIDER_ADD */
typedef struct blade_protocol_provider_rank_update_param_s {
	int rank;
} blade_protocol_provider_rank_update_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM, blade_protocol_provider_rank_update_param_t)
	SWCLT_JSON_MARSHAL_INT(rank)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM, blade_protocol_provider_rank_update_param_t)
	SWCLT_JSON_DESTROY_INT(rank)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM, blade_protocol_provider_rank_update_param_t)
	SWCLT_JSON_PARSE_INT_OPT_DEF(rank, 1)
SWCLT_JSON_PARSE_END()



/**
 * CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD_ASYNC
 * Creates a command which holds and owns the request json for a protocol provider add
 * request, also takes owner of the provider data and channels json passed in
 */
static inline swclt_cmd_t *CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char *protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
	ks_json_t **channels,
	int rank,
	ks_json_t **data)
{
	swclt_cmd_t *cmd = NULL;
	ks_status_t status;
	ks_json_t *params;
	ks_json_t *request;

	if (!(params = BLADE_PROTOCOL_PROVIDER_ADD_PARAM_MARSHAL(&(blade_protocol_provider_add_param_t){
																 default_method_execute_access,
																 default_channel_subscribe_access,
																 default_channel_broadcast_access,
																 methods ? *methods : NULL,
																 channels ? *channels : NULL,
																 rank,
 																 data ? *data : NULL}))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol request params");

		/* Since provider_data and channels was at the end of the macro declartaion, it will have
		 * not touched it and we will not indicate we've taken ownership of it */
		return cmd;
	}
											  
	/* We have taken ownership of the provider_data and channels from here on out */
	if (data) *data = NULL;
	if (channels) *channels = NULL;
	
	if (!(request = BLADE_PROTOCOL_RQU_MARSHAL(
			&(blade_protocol_rqu_t){
				BLADE_PROTOCOL_CMD_PROVIDER_ADD,
				protocol,
				params}))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol request");

		/* Since params is allocated in the same pool, nothing to worry about with ownership */
		ks_json_delete(&params);
		return cmd;
	}

	/* Now hand it to the command, it will take ownership of it if successful
	 * and null out our ptr */
	if ((status = swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_PROTOCOL_METHOD,
			&request,
			BLADE_PROTOCOL_TTL_MS,
			BLADE_PROTOCOL_FLAGS,
			ks_uuid_null()))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol cmd: %lu", status);

		/* Safe to free this or at least attempt to, cmd will have set it to null if it
		 * took ownership of it */
		ks_json_delete(&request);
		return cmd;
	}

	/* Phew, successfully allocated the command */
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD(
	const char *protocol,
	blade_access_control_t default_method_execute_access,
	blade_access_control_t default_channel_subscribe_access,
	blade_access_control_t default_channel_broadcast_access,
	ks_json_t **methods,
 	ks_json_t **channels,
	int rank,
	ks_json_t **data)
{
	return CREATE_BLADE_PROTOCOL_PROVIDER_ADD_CMD_ASYNC(
		NULL,
		NULL,
		protocol,
		default_method_execute_access,
		default_channel_subscribe_access,
		default_channel_broadcast_access,
		methods,
		channels,
		rank,
		data);
}


/**
 * CREATE_BLADE_PROTOCOL_PROVIDER_REMOVE_CMD_ASYNC
 * Creates a command which holds and owns the request json for a protocol provider add
 * request, also takes owner of the provider data and channels json passed in
 */
static inline swclt_cmd_t *CREATE_BLADE_PROTOCOL_PROVIDER_REMOVE_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char *protocol)
{
	swclt_cmd_t *cmd = NULL;
	ks_status_t status;
	ks_json_t *request;

	if (!(request = BLADE_PROTOCOL_RQU_MARSHAL(
			&(blade_protocol_rqu_t){
				BLADE_PROTOCOL_CMD_PROVIDER_REMOVE,
				protocol,
				NULL}))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol request");

		/* Since params is allocated in the same pool, nothing to worry about with ownership */
		return cmd;
	}

	/* Now hand it to the command, it will take ownership of it if successful
	 * and null out our ptr */
	if ((status = swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_PROTOCOL_METHOD,
			&request,
			BLADE_PROTOCOL_TTL_MS,
			BLADE_PROTOCOL_FLAGS,
			ks_uuid_null()))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol cmd: %lu", status);

		/* Safe to free this or at least attempt to, cmd will have set it to null if it
		 * took ownership of it */
		ks_json_delete(&request);
		return cmd;
	}

	/* Phew, successfully allocated the command */
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_PROTOCOL_PROVIDER_REMOVE_CMD(const char *protocol)
{
	return CREATE_BLADE_PROTOCOL_PROVIDER_REMOVE_CMD_ASYNC(
		NULL,
		NULL,
		protocol);
}

/**
 * CREATE_BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_CMD_ASYNC
 * Creates a command which holds and owns the request json for a protocol provider rank update request
 */
static inline swclt_cmd_t *CREATE_BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_CMD_ASYNC(
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char *protocol,
	int rank)
{
	swclt_cmd_t *cmd = NULL;
	ks_status_t status;
	ks_json_t *params;
	ks_json_t *request;

	if (!(params = BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM_MARSHAL(&(blade_protocol_provider_rank_update_param_t){ rank }))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol request params");

		/* Since provider_data and channels was at the end of the macro declartaion, it will have
		 * not touched it and we will not indicate we've taken ownership of it */
		return cmd;
	}
											  
	/* We have taken ownership of the provider_data and channels from here on out */
	if (!(request = BLADE_PROTOCOL_RQU_MARSHAL(
			&(blade_protocol_rqu_t){
				BLADE_PROTOCOL_CMD_PROVIDER_RANK_UPDATE,
				protocol,
				params}))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol request");

		/* Since params is allocated in the same pool, nothing to worry about with ownership */
		ks_json_delete(&params);
		return cmd;
	}

	/* Now hand it to the command, it will take ownership of it if successful
	 * and null out our ptr */
	if ((status = swclt_cmd_create_ex(
			&cmd,
			cb,
			cb_data,
			BLADE_PROTOCOL_METHOD,
			&request,
			BLADE_PROTOCOL_TTL_MS,
			BLADE_PROTOCOL_FLAGS,
			ks_uuid_null()))) {
		ks_log(KS_LOG_WARNING, "Failed to allocate protocol cmd: %lu", status);

		/* Safe to free this or at least attempt to, cmd will have set it to null if it
		 * took ownership of it */
		ks_json_delete(&request);
		return cmd;
	}

	/* Phew, successfully allocated the command */
	return cmd;
}

static inline swclt_cmd_t *CREATE_BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_CMD(
	const char *protocol,
	int rank)
{
	return CREATE_BLADE_PROTOCOL_PROVIDER_RANK_UPDATE_CMD_ASYNC(
		NULL,
		NULL,
		protocol,
		rank);
}


