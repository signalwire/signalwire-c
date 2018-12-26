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

typedef enum {
	BLADE_ACL_NONE = -1,
	BLADE_ACL_SYSTEM = 0,		/* Only certified nodes */
	BLADE_ACL_PUBLIC = 1,		/* Anyone */
	BLADE_ACL_RESTRICTED = 2,	/* Some uncertified nodes, depending on implementation of authenticator */
} blade_access_control_t;

typedef struct blade_channel_s {
	const char * name;
	blade_access_control_t broadcast_access;
	blade_access_control_t subscribe_access;
} blade_channel_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_CHANNEL, blade_channel_t)
	SWCLT_JSON_MARSHAL_STRING(name)
	SWCLT_JSON_MARSHAL_INT(broadcast_access)
	SWCLT_JSON_MARSHAL_INT(subscribe_access)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_CHANNEL, blade_channel_t)
	SWCLT_JSON_DESTROY_STRING(name)
	SWCLT_JSON_DESTROY_INT(broadcast_access)
	SWCLT_JSON_DESTROY_INT(subscribe_access)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_CHANNEL, blade_channel_t)
	SWCLT_JSON_PARSE_STRING(name)
	SWCLT_JSON_PARSE_INT(broadcast_access)
	SWCLT_JSON_PARSE_INT(subscribe_access)
SWCLT_JSON_PARSE_END()

/* Define our version structure */
typedef struct blade_version_s {
	uint32_t major;
	uint32_t minor;
	uint32_t revision;
} blade_version_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_VERSION, blade_version_t)
	SWCLT_JSON_MARSHAL_INT(major)
	SWCLT_JSON_MARSHAL_INT(minor)
	SWCLT_JSON_MARSHAL_INT(revision)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_VERSION, blade_version_t)
	SWCLT_JSON_DESTROY_INT(major)
	SWCLT_JSON_DESTROY_INT(minor)
	SWCLT_JSON_DESTROY_INT(revision)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_ALLOC_BEG(BLADE_VERSION, blade_version_t)
	SWCLT_JSON_ALLOC_DEFAULT(major, SWCLT_BLADE_VERSION_MAJOR)
	SWCLT_JSON_ALLOC_DEFAULT(minor, SWCLT_BLADE_VERSION_MINOR)
	SWCLT_JSON_ALLOC_DEFAULT(revision, SWCLT_BLADE_VERSION_REVISION)
SWCLT_JSON_ALLOC_END()

SWCLT_JSON_PARSE_BEG(BLADE_VERSION, blade_version_t)
	SWCLT_JSON_PARSE_INT(major)
	SWCLT_JSON_PARSE_INT(minor)
	SWCLT_JSON_PARSE_INT(revision)
SWCLT_JSON_PARSE_END()

typedef struct blade_node_s {
	const char *nodeid;
	ks_bool_t certified;
} blade_node_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_NODE, blade_node_t)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_BOOL(certified)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_NODE, blade_node_t)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_BOOL(certified)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_NODE, blade_node_t)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_BOOL_OPT(certified)
SWCLT_JSON_PARSE_END()

typedef struct blade_provider_s {
	const char *nodeid;
	ks_json_t *identities;	/* list of identity uri's string's */
	int rank;
	ks_json_t *data;
} blade_provider_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_PROVIDER, blade_provider_t)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_ITEM_OPT(identities)
	SWCLT_JSON_MARSHAL_INT(rank)
	SWCLT_JSON_MARSHAL_ITEM_OPT(data)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_PROVIDER, blade_provider_t)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_ITEM(identities)
	SWCLT_JSON_DESTROY_INT(rank)
	SWCLT_JSON_DESTROY_ITEM(data)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_PROVIDER, blade_provider_t)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_ITEM_OPT(identities)
	SWCLT_JSON_PARSE_INT_OPT_DEF(rank, 1)
	SWCLT_JSON_PARSE_ITEM_OPT(data)
SWCLT_JSON_PARSE_END()

typedef struct blade_subscription_s {
	const char *protocol;
	const char *channel;
	ks_json_t *subscribers;	/* list of identities */
} blade_subscription_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_SUBSCRIPTION, blade_subscription_t)
	SWCLT_JSON_MARSHAL_STRING(protocol)
	SWCLT_JSON_MARSHAL_STRING(channel)
	SWCLT_JSON_MARSHAL_ITEM(subscribers)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_SUBSCRIPTION, blade_subscription_t)
	SWCLT_JSON_DESTROY_STRING(protocol)
	SWCLT_JSON_DESTROY_STRING(channel)
	SWCLT_JSON_DESTROY_ITEM(subscribers)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_SUBSCRIPTION, blade_subscription_t)
	SWCLT_JSON_PARSE_STRING(protocol)
	SWCLT_JSON_PARSE_STRING(channel)
	SWCLT_JSON_PARSE_ITEM(subscribers)
SWCLT_JSON_PARSE_END()

typedef struct blade_protocol_s {
	const char *name;
	uint32_t default_method_execute_access;
	uint32_t default_channel_broadcast_access;
	uint32_t default_channel_subscribe_access;
	ks_json_t *providers;	/* list of blade_provider_t's */
	ks_json_t *channels;		/* list of blade_channel_t's */
} blade_protocol_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_PROTOCOL, blade_protocol_t)
	SWCLT_JSON_MARSHAL_STRING(name)
	SWCLT_JSON_MARSHAL_INT(default_method_execute_access)
	SWCLT_JSON_MARSHAL_INT(default_channel_broadcast_access)
	SWCLT_JSON_MARSHAL_INT(default_channel_subscribe_access)
	SWCLT_JSON_MARSHAL_ITEM(providers)
	SWCLT_JSON_MARSHAL_ITEM(channels)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_PROTOCOL, blade_protocol_t)
	SWCLT_JSON_DESTROY_STRING(name)
	SWCLT_JSON_DESTROY_INT(default_method_execute_access)
	SWCLT_JSON_DESTROY_INT(default_channel_broadcast_access)
	SWCLT_JSON_DESTROY_INT(default_channel_subscribe_access)
	SWCLT_JSON_DESTROY_ITEM(providers)
	SWCLT_JSON_DESTROY_ITEM(channels)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_PROTOCOL, blade_protocol_t)
	SWCLT_JSON_PARSE_STRING(name)
	SWCLT_JSON_PARSE_INT(default_method_execute_access)
	SWCLT_JSON_PARSE_INT(default_channel_broadcast_access)
	SWCLT_JSON_PARSE_INT(default_channel_subscribe_access)
	SWCLT_JSON_PARSE_ITEM(providers)
	SWCLT_JSON_PARSE_ITEM(channels)
SWCLT_JSON_PARSE_END()

KS_END_EXTERN_C

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
