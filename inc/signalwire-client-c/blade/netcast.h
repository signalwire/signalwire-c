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

/* Define all our commands */
#define BLADE_NETCAST_CMD_ROUTE_ADD "route.add"
#define BLADE_NETCAST_CMD_ROUTE_REMOVE "route.remove"
#define BLADE_NETCAST_CMD_IDENTITY_ADD "identity.add"
#define BLADE_NETCAST_CMD_IDENTITY_REMOVE "identity.remove"
#define BLADE_NETCAST_CMD_PROTOCOL_ADD "protocol.add"
#define BLADE_NETCAST_CMD_PROTOCOL_REMOVE "protocol.remove"
#define BLADE_NETCAST_CMD_PROTOCOL_UPDATE "protocol.update"
#define BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD "protocol.provider.add"
#define BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE "protocol.provider.remove"
#define BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE "protocol.provider.rank.update"
#define BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE "protocol.provider.data.update"
#define BLADE_NETCAST_CMD_PROTOCOL_CHANNEL_ADD "protocol.channel.add"
#define BLADE_NETCAST_CMD_PROTOCOL_CHANNEL_REMOVE "protocol.channel.remove"
#define BLADE_NETCAST_CMD_SUBSCRIPTION_ADD "subscription.add"
#define BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE "subscription.remove"
#define BLADE_NETCAST_CMD_AUTHORITY_ADD "authority.add"
#define BLADE_NETCAST_CMD_AUTHORITY_REMOVE "authority.remove"
#define BLADE_NETCAST_CMD_AUTHORIZATION_ADD "authorization.add"
#define BLADE_NETCAST_CMD_AUTHORIZATION_UPDATE "authorization.update"
#define BLADE_NETCAST_CMD_AUTHORIZATION_REMOVE "authorization.remove"
#define BLADE_NETCAST_CMD_ACCESS_ADD "access.add"
#define BLADE_NETCAST_CMD_ACCESS_REMOVE "access.remove"

/* The method name for a netcast request */
#define BLADE_NETCAST_METHOD "blade.netcast"

/* Flags for the command, in our case we don't get replies */
#define BLADE_NETCAST_FLAGS  SWCLT_CMD_FLAG_NOREPLY

/* Create our netcast request template */
typedef struct blade_netcast_rqu_s {
	const char *command;
	ks_bool_t certified_only;
	const char *netcaster_nodeid;
	ks_json_t *params;
} blade_netcast_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_RQU, blade_netcast_rqu_t)
	SWCLT_JSON_MARSHAL_STRING(command)
	SWCLT_JSON_MARSHAL_BOOL(certified_only)
	SWCLT_JSON_MARSHAL_STRING(netcaster_nodeid)
	SWCLT_JSON_MARSHAL_ITEM(params)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_RQU, blade_netcast_rqu_t)
	SWCLT_JSON_DESTROY_STRING(command)
	SWCLT_JSON_DESTROY_BOOL(certified_only)
	SWCLT_JSON_DESTROY_STRING(netcaster_nodeid)
	SWCLT_JSON_DESTROY_ITEM(params)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_RQU, blade_netcast_rqu_t)
	SWCLT_JSON_PARSE_STRING(command)
	SWCLT_JSON_PARSE_BOOL_OPT(certified_only)
	SWCLT_JSON_PARSE_STRING(netcaster_nodeid)
	SWCLT_JSON_PARSE_ITEM(params)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_PROTOCOL_ADD */
typedef struct blade_netcast_protocol_add_param_s {
	const char *protocol;
} blade_netcast_protocol_add_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_PROTOCOL_ADD_PARAM, blade_netcast_protocol_add_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_PROTOCOL_ADD_PARAM, blade_netcast_protocol_add_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_PROTOCOL_ADD_PARAM, blade_netcast_protocol_add_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_PROTOCOL_REMOVE */
typedef struct blade_netcast_protocol_remove_param_s {
	const char *protocol;
} blade_netcast_protocol_remove_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_PROTOCOL_REMOVE_PARAM, blade_netcast_protocol_remove_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_PROTOCOL_REMOVE_PARAM, blade_netcast_protocol_remove_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_PROTOCOL_REMOVE_PARAM, blade_netcast_protocol_remove_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD */
typedef struct blade_netcast_protocol_provider_add_param_s {
	const char *protocol;
	const char *nodeid;
	blade_access_control_t default_method_execute_access;
	blade_access_control_t default_channel_subscribe_access;
	blade_access_control_t default_channel_broadcast_access;
	ks_json_t *channels;
	int rank;
	ks_json_t *data;
} blade_netcast_protocol_provider_add_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM, blade_netcast_protocol_provider_add_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_INT(default_method_execute_access)
	SWCLT_JSON_MARSHAL_INT(default_channel_subscribe_access)
	SWCLT_JSON_MARSHAL_INT(default_channel_broadcast_access)
	SWCLT_JSON_MARSHAL_ITEM_OPT(channels)
	SWCLT_JSON_MARSHAL_INT(rank)
	SWCLT_JSON_MARSHAL_ITEM_OPT(data)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM, blade_netcast_protocol_provider_add_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_INT(default_method_execute_access)
	SWCLT_JSON_DESTROY_INT(default_channel_subscribe_access)
	SWCLT_JSON_DESTROY_INT(default_channel_broadcast_access)
	SWCLT_JSON_DESTROY_ITEM(channels)
	SWCLT_JSON_DESTROY_INT(rank)
	SWCLT_JSON_DESTROY_ITEM(data)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM, blade_netcast_protocol_provider_add_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_INT_OPT_DEF(default_method_execute_access, BLADE_ACL_SYSTEM)
	SWCLT_JSON_PARSE_INT_OPT_DEF(default_channel_subscribe_access, BLADE_ACL_SYSTEM)
	SWCLT_JSON_PARSE_INT_OPT_DEF(default_channel_broadcast_access, BLADE_ACL_SYSTEM)
	SWCLT_JSON_PARSE_ITEM_OPT(channels)
	SWCLT_JSON_PARSE_INT_OPT_DEF(rank, 1)
	SWCLT_JSON_PARSE_ITEM_OPT(data)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_REMOVE */
typedef struct blade_netcast_protocol_provider_remove_param_s {
	const char *protocol;
	const char *nodeid;
} blade_netcast_protocol_provider_remove_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_REMOVE_PARAM, blade_netcast_protocol_provider_remove_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_REMOVE_PARAM, blade_netcast_protocol_provider_remove_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(nodeid)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_REMOVE_PARAM, blade_netcast_protocol_provider_remove_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(nodeid)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_RANK_UPDATE */
typedef struct blade_netcast_protocol_provider_rank_update_param_s {
	const char *protocol;
	const char *nodeid;
	int rank;
} blade_netcast_protocol_provider_rank_update_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM, blade_netcast_protocol_provider_rank_update_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_INT(rank)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM, blade_netcast_protocol_provider_rank_update_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_INT(rank)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_RANK_UPDATE_PARAM, blade_netcast_protocol_provider_rank_update_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_INT_OPT_DEF(rank, 1)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_DATA_UPDATE */
typedef struct blade_netcast_protocol_provider_data_update_param_s {
	const char *protocol;
	const char *nodeid;
	ks_json_t *data;
} blade_netcast_protocol_provider_data_update_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_DATA_UPDATE_PARAM, blade_netcast_protocol_provider_data_update_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_ITEM_OPT(data)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_DATA_UPDATE_PARAM, blade_netcast_protocol_provider_data_update_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_ITEM(data)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_PROTOCOL_PROVIDER_DATA_UPDATE_PARAM, blade_netcast_protocol_provider_data_update_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_ITEM_OPT(data)
SWCLT_JSON_PARSE_END()


/* The params definition for BLADE_NETCAST_CMD_AUTHORITY_ADD */
typedef struct blade_netcast_authority_add_param_s {
	const char *nodeid;
} blade_netcast_authority_add_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_AUTHORITY_ADD_PARAM, blade_netcast_authority_add_param_t)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_AUTHORITY_ADD_PARAM, blade_netcast_authority_add_param_t)
	SWCLT_JSON_DESTROY_STRING(nodeid)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_AUTHORITY_ADD_PARAM, blade_netcast_authority_add_param_t)
	SWCLT_JSON_PARSE_STRING(nodeid)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_AUTHORITY_REMOVE */
typedef struct blade_netcast_authority_remove_param_s {
	const char *nodeid;
} blade_netcast_authority_remove_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_AUTHORITY_REMOVE_PARAM, blade_netcast_authority_remove_param_t)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_AUTHORITY_REMOVE_PARAM, blade_netcast_authority_remove_param_t)
	SWCLT_JSON_DESTROY_STRING(nodeid)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_AUTHORITY_REMOVE_PARAM, blade_netcast_authority_remove_param_t)
	SWCLT_JSON_PARSE_STRING(nodeid)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_SUBSCRIPTION_ADD */
typedef struct blade_netcast_subscription_add_param_s {
	const char *protocol;
	const char *nodeid;
	ks_json_t *channels;
} blade_netcast_subscription_add_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_SUBSCRIPTION_ADD_PARAM, blade_netcast_subscription_add_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_ITEM_OPT(channels)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_SUBSCRIPTION_ADD_PARAM, blade_netcast_subscription_add_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_ITEM(channels)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_SUBSCRIPTION_ADD_PARAM, blade_netcast_subscription_add_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_ITEM_OPT(channels)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_SUBSCRIPTION_REMOVE */
typedef struct blade_netcast_subscription_remove_param_s {
	const char *protocol;
	const char *nodeid;
	ks_json_t *channels;
} blade_netcast_subscription_remove_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_SUBSCRIPTION_REMOVE_PARAM, blade_netcast_subscription_remove_param_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_ITEM_OPT(channels)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_SUBSCRIPTION_REMOVE_PARAM, blade_netcast_subscription_remove_param_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_ITEM(channels)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_SUBSCRIPTION_REMOVE_PARAM, blade_netcast_subscription_remove_param_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_ITEM_OPT(channels)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_ROUTE_ADD */
typedef blade_node_t blade_netcast_route_add_param_t;

#define BLADE_NETCAST_ROUTE_ADD_PARAM_MARSHAL BLADE_NODE_MARSHAL
#define BLADE_NETCAST_ROUTE_ADD_PARAM_DESTROY BLADE_NODE_DESTROY
#define BLADE_NETCAST_ROUTE_ADD_PARAM_PARSE BLADE_NODE_PARSE

/* The params definition for BLADE_NETCAST_CMD_ROUTE_REMOVE */
typedef blade_node_t blade_netcast_route_remove_param_t;

#define BLADE_NETCAST_ROUTE_REMOVE_PARAM_MARSHAL BLADE_NODE_MARSHAL
#define BLADE_NETCAST_ROUTE_REMOVE_PARAM_DESTROY BLADE_NODE_DESTROY
#define BLADE_NETCAST_ROUTE_REMOVE_PARAM_PARSE BLADE_NODE_PARSE

/* The params definition for BLADE_NETCAST_CMD_IDENTITY_ADD */
typedef struct blade_netcast_identity_add_param_s {
	const char *nodeid;
	const char *identity;
} blade_netcast_identity_add_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_IDENTITY_ADD_PARAM, blade_netcast_identity_add_param_t)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_STRING(identity)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_IDENTITY_ADD_PARAM, blade_netcast_identity_add_param_t)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_STRING(identity)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_IDENTITY_ADD_PARAM, blade_netcast_identity_add_param_t)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_STRING(identity)
SWCLT_JSON_PARSE_END()

/* The params definition for BLADE_NETCAST_CMD_IDENTITY_REMOVE */
typedef struct blade_netcast_identity_remove_param_s {
	const char *nodeid;
	const char *identity;
} blade_netcast_identity_remove_param_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NETCAST_IDENTITY_REMOVE_PARAM, blade_netcast_identity_remove_param_t)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_STRING(identity)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NETCAST_IDENTITY_REMOVE_PARAM, blade_netcast_identity_remove_param_t)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_STRING(identity)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NETCAST_IDENTITY_REMOVE_PARAM, blade_netcast_identity_remove_param_t)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_STRING(identity)
SWCLT_JSON_PARSE_END()

