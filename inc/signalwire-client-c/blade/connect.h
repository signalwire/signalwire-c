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

/* The method name for a connect request */
static const char *BLADE_CONNECT_METHOD = "blade.connect";

/* Flags for the command */
#define BLADE_CONNECT_FLAGS 0

/* Default time to live for connect */
#define BLADE_CONNECT_TTL_MS	BLADE_DEFAULT_CMD_TTL_MS

/* Setup the target blade version we will support */
#define BLADE_VERSION_MAJOR			2
#define BLADE_VERSION_MINOR			0
#define BLADE_VERSION_REVISION		0

/* Define our request structure */
typedef struct blade_connect_rqu_s {
	blade_version_t *version;
	ks_uuid_t sessionid;
	ks_json_t *authentication;
} blade_connect_rqu_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_CONNECT_RQU, blade_connect_rqu_t)
	SWCLT_JSON_MARSHAL_CUSTOM(BLADE_VERSION, version)
	SWCLT_JSON_MARSHAL_UUID(sessionid)
	SWCLT_JSON_MARSHAL_ITEM_OPT(authentication)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_CONNECT_RQU, blade_connect_rqu_t)
	SWCLT_JSON_DESTROY_CUSTOM(BLADE_VERSION, version)
	SWCLT_JSON_DESTROY_UUID(sessionid)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_ALLOC_BEG(BLADE_CONNECT_RQU, blade_connect_rqu_t)
	SWCLT_JSON_ALLOC_CUSTOM(BLADE_VERSION, version)
SWCLT_JSON_ALLOC_END()

SWCLT_JSON_PARSE_BEG(BLADE_CONNECT_RQU, blade_connect_rqu_t)
	SWCLT_JSON_PARSE_CUSTOM(BLADE_VERSION, version)
	SWCLT_JSON_PARSE_UUID(sessionid)
	SWCLT_JSON_PARSE_ITEM_OPT(authentication)
SWCLT_JSON_PARSE_END()

/* Define our reply structure */
typedef struct blade_connect_rpl_s {
	ks_bool_t session_restored;
	ks_uuid_t sessionid;
	const char *nodeid;
	const char *master_nodeid;
	ks_json_t *authorization;
	ks_json_t *routes;
	ks_json_t *protocols;
	ks_json_t *subscriptions;
	ks_json_t *authorities;
	ks_json_t *protocols_uncertified;
} blade_connect_rpl_t;

SWCLT_JSON_MARSHAL_BEG(BLADE_CONNECT_RPL, blade_connect_rpl_t)
    SWCLT_JSON_MARSHAL_BOOL(session_restored)
	SWCLT_JSON_MARSHAL_UUID(sessionid)
	SWCLT_JSON_MARSHAL_STRING(nodeid)
	SWCLT_JSON_MARSHAL_STRING(master_nodeid)
	SWCLT_JSON_MARSHAL_ITEM_OPT(authorization)
	SWCLT_JSON_MARSHAL_ITEM_OPT(routes)
	SWCLT_JSON_MARSHAL_ITEM_OPT(protocols)
	SWCLT_JSON_MARSHAL_ITEM_OPT(subscriptions)
	SWCLT_JSON_MARSHAL_ITEM_OPT(authorities)
	SWCLT_JSON_MARSHAL_ITEM_OPT(protocols_uncertified)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BLADE_CONNECT_RPL, blade_connect_rpl_t)
    SWCLT_JSON_DESTROY_BOOL(session_restored)
    SWCLT_JSON_DESTROY_UUID(sessionid)
	SWCLT_JSON_DESTROY_STRING(nodeid)
	SWCLT_JSON_DESTROY_STRING(master_nodeid)
	SWCLT_JSON_DESTROY_ITEM(authorization)
	SWCLT_JSON_DESTROY_ITEM(routes)
	SWCLT_JSON_DESTROY_ITEM(protocols)
	SWCLT_JSON_DESTROY_ITEM(subscriptions)
	SWCLT_JSON_DESTROY_ITEM(authorities)
	SWCLT_JSON_DESTROY_ITEM(protocols_uncertified)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BLADE_CONNECT_RPL, blade_connect_rpl_t)
    SWCLT_JSON_PARSE_BOOL(session_restored)
	SWCLT_JSON_PARSE_UUID(sessionid)
	SWCLT_JSON_PARSE_STRING(nodeid)
	SWCLT_JSON_PARSE_STRING(master_nodeid)
	SWCLT_JSON_PARSE_ITEM_OPT(authorization)
	SWCLT_JSON_PARSE_ITEM_OPT(routes)
	SWCLT_JSON_PARSE_ITEM_OPT(protocols)
	SWCLT_JSON_PARSE_ITEM_OPT(subscriptions)
	SWCLT_JSON_PARSE_ITEM_OPT(authorities)
	SWCLT_JSON_PARSE_ITEM_OPT(protocols_uncertified)
SWCLT_JSON_PARSE_END()

/**
 * CREATE_BLADE_CONNECT_CMD_ASYNC - Creates a command which holds
 * and owns the request json for a connect request.
 */
static inline swclt_cmd_t CREATE_BLADE_CONNECT_CMD_ASYNC(
	ks_uuid_t previous_sessionid,
	ks_json_t **authentication,
	swclt_cmd_cb_t cb,
	void *cb_data)
{
	blade_connect_rqu_t *connect_rqu = NULL;
	swclt_cmd_t cmd = KS_NULL_HANDLE;
	ks_json_t *obj = NULL;
	ks_pool_t *pool;

	if (ks_pool_open(&pool))
		goto done;

	if (BLADE_CONNECT_RQU_ALLOC(pool, &connect_rqu))
		goto done;

	connect_rqu->sessionid = previous_sessionid;
	if (authentication && *authentication) {
		connect_rqu->authentication = *authentication;
		*authentication = NULL;
	}

	if (!(obj = BLADE_CONNECT_RQU_MARSHAL(pool, connect_rqu)))
		goto done;

	if (swclt_cmd_create_ex(
			&cmd,
			&pool,
			cb,
			cb_data,
			BLADE_CONNECT_METHOD,
			&obj,
			BLADE_CONNECT_TTL_MS,
			BLADE_CONNECT_FLAGS,
			ks_uuid_null()))
		goto done;

done:
	BLADE_CONNECT_RQU_DESTROY(&connect_rqu);
	ks_json_delete(&obj);
	ks_pool_close(&pool);

	return cmd;
}

static inline swclt_cmd_t CREATE_BLADE_CONNECT_CMD(ks_uuid_t previous_sessionid,
												   ks_json_t **authentication)
{
	return CREATE_BLADE_CONNECT_CMD_ASYNC(previous_sessionid, authentication, NULL, NULL);
}
