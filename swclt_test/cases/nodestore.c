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

#include "swclt_test.h"
#include "signalwire-client-c/internal/nodestore.h"

ks_uuid_t g_route_nodeid_1, g_route_nodeid_2, g_sessionid;

blade_netcast_rqu_t __netcast_protocol_provider_add_request(ks_pool_t *pool, const char *protocol, ks_uuid_t nodeid, const char *channel)
{
	blade_netcast_rqu_t request;
	request.command = BLADE_NETCAST_CMD_PROTOCOL_PROVIDER_ADD;
	request.certified_only = KS_TRUE;
	request.netcaster_nodeid = ks_uuid_null_str(pool);

	/* Fill in the params too */
	blade_netcast_protocol_provider_add_param_t params;
	params.protocol = protocol;
	params.nodeid = ks_uuid_str(pool, &nodeid);
	params.channels = ks_json_pcreate_array_inline(pool, 1,
		BLADE_CHANNEL_MARSHAL(pool, &(blade_channel_t){channel, BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC}));

	/* Marshal it into its parent request */
	request.params = BLADE_NETCAST_PROTOCOL_PROVIDER_ADD_PARAM_MARSHAL(pool, &params);

	return request;
}

blade_netcast_rqu_t __netcast_route_add_request(ks_pool_t *pool, const char *nodeid, ks_bool_t certified)
{
	blade_netcast_rqu_t request;
	request.command = BLADE_NETCAST_CMD_ROUTE_ADD;
	request.certified_only = KS_TRUE;
	request.netcaster_nodeid = ks_uuid_null_str(pool);

	/* Fill in the params too */
	blade_netcast_route_add_param_t params;
	params.certified = certified;
	params.nodeid = nodeid;

	/* Marshal it into its parent request */
	request.params = BLADE_NETCAST_ROUTE_ADD_PARAM_MARSHAL(pool, &params);

	return request;
}

blade_netcast_rqu_t __netcast_route_remove_request(ks_pool_t *pool, const char *nodeid)
{
	blade_netcast_rqu_t request;
	request.command = BLADE_NETCAST_CMD_ROUTE_REMOVE;
	request.certified_only = KS_TRUE;
	request.netcaster_nodeid = ks_uuid_null_str(pool);

	/* Fill in the params too */
	blade_netcast_route_remove_param_t params;
	params.nodeid = nodeid;
	params.certified = KS_TRUE;

	/* Marshal it into its parent request */
	request.params = BLADE_NETCAST_ROUTE_REMOVE_PARAM_MARSHAL(pool, &params);

	return request;
}

blade_connect_rpl_t __connect_reply(ks_pool_t *pool)
{
	ks_json_t *routes = ks_json_pcreate_array(pool);
	ks_json_t *protocols = ks_json_pcreate_array(pool);
	ks_json_t *subscriptions = ks_json_pcreate_array(pool);

	/* Add a couple routes */
	ks_json_add_item_to_array(routes, BLADE_NODE_MARSHAL(pool, &(blade_node_t){ks_uuid_str(pool, &g_route_nodeid_1)}));
	ks_json_add_item_to_array(routes, BLADE_NODE_MARSHAL(pool, &(blade_node_t){ks_uuid_str(pool, &g_route_nodeid_2)}));

	/* Add a couple protocols each with one provider and one channel */
	ks_json_add_item_to_array(protocols,
		BLADE_PROTOCOL_MARSHAL(pool, &(blade_protocol_t){
				"bobo_protocol_1", BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC,
				ks_json_pcreate_array_inline(pool, 1, BLADE_PROVIDER_MARSHAL(pool, &(blade_provider_t){ks_uuid_str(pool, &g_route_nodeid_1), ks_json_pcreate_array(pool)})),
				ks_json_pcreate_array_inline(pool, 1, BLADE_CHANNEL_MARSHAL(pool, &(blade_channel_t){"bobo_channel_1", BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC}))
			}));

	ks_json_add_item_to_array(protocols,
		BLADE_PROTOCOL_MARSHAL(pool, &(blade_protocol_t){
				"bobo_protocol_2", BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC,
				ks_json_pcreate_array_inline(pool, 1, BLADE_PROVIDER_MARSHAL(pool, &(blade_provider_t){ks_uuid_str(pool, &g_route_nodeid_2), ks_json_pcreate_array(pool)})),
				ks_json_pcreate_array_inline(pool, 1, BLADE_CHANNEL_MARSHAL(pool, &(blade_channel_t){"bobo_channel_2", BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC}))
			}));

	/* Have the second node be subscribed to the first */
	ks_json_add_item_to_array(subscriptions,
		BLADE_SUBSCRIPTION_MARSHAL(pool, &(blade_subscription_t){
			"bobo_protocol_1",
			"bobo_channel_1",
			ks_json_pcreate_array_inline(pool, 1,
				ks_json_pcreate_uuid(pool, g_route_nodeid_2))
		}));

	/* Now compose it altogether in a connect result */
	blade_connect_rpl_t reply = (blade_connect_rpl_t){
            KS_FALSE,
			*ks_uuid(&g_sessionid),
			ks_uuid_str(pool, &g_route_nodeid_1),
			ks_uuid_null_str(pool),
			NULL,
			routes,
			protocols,
			subscriptions,
			ks_json_pcreate_array(pool)
		};
	return reply;
}

void test_nodestore_update(ks_pool_t *pool)
{
	blade_connect_rpl_t connect_rpl = __connect_reply(pool);
	swclt_store_t store;
	swclt_store_ctx_t *store_ctx;
	ks_uuid_t new_route_nodeid;
	const char *new_route_nodeid_str;

	ks_uuid(&new_route_nodeid);
	new_route_nodeid_str = ks_uuid_str(pool, &new_route_nodeid);

	REQUIRE(!swclt_store_create(&store));
	REQUIRE(!swclt_store_populate(store, &connect_rpl));
	REQUIRE(!swclt_store_get(store, &store_ctx));

	/* The store should properly render the types */
	REQUIRE(ks_hash_count(store_ctx->protocols) == 2);
	REQUIRE(ks_hash_count(store_ctx->subscriptions) == 1);
	REQUIRE(ks_hash_count(store_ctx->routes) == 2);
	REQUIRE(ks_hash_count(store_ctx->authorities) == 0);

	/* Now update with a new node */
	blade_netcast_rqu_t netcast_rqu = __netcast_route_add_request(pool, new_route_nodeid_str, KS_TRUE);
	REQUIRE(!swclt_store_update(store, &netcast_rqu));
	REQUIRE(ks_hash_count(store_ctx->routes) == 3);
	REQUIRE(((blade_node_t *)ks_hash_search(store_ctx->routes, new_route_nodeid_str, KS_UNLOCKED))->certified == KS_TRUE);
	ks_json_delete(&netcast_rqu.params);

	/* And a new provider with a new provider + channel */
	netcast_rqu = __netcast_protocol_provider_add_request(pool, "bobo_protocol_new", new_route_nodeid, "bobo_channel_new");
	REQUIRE(!swclt_store_update(store, &netcast_rqu));
	REQUIRE(ks_hash_count(store_ctx->protocols) == 3);
	{
		blade_protocol_t *protocol = (blade_protocol_t *)ks_hash_search(store_ctx->protocols, "bobo_protocol_new", KS_UNLOCKED);
		REQUIRE(!strcmp(protocol->name, "bobo_protocol_new"));

		/* Should have one channel in it called something silly */
		REQUIRE(ks_json_get_array_size(protocol->channels) == 1);

		{
			blade_channel_t *channel;
			REQUIRE(!BLADE_CHANNEL_PARSE(pool, ks_json_get_array_item(protocol->channels, 0), &channel));
			REQUIRE(!strcmp(channel->name, "bobo_channel_new"));
			BLADE_CHANNEL_DESTROY(&channel);
		}
	}
	ks_json_delete(&netcast_rqu.params);

	/* And remove it, should also remove the protocol */
	netcast_rqu = __netcast_route_remove_request(pool, new_route_nodeid_str);
	REQUIRE(!swclt_store_update(store, &netcast_rqu));
	REQUIRE(ks_hash_count(store_ctx->routes) == 2);
	REQUIRE(ks_hash_count(store_ctx->protocols) == 2);
	ks_json_delete(&netcast_rqu.params);

	swclt_store_put(&store_ctx);
	ks_handle_destroy(&store);
}

void test_nodestore_protocol_select(ks_pool_t *pool)
{

}

void test_nodestore(ks_pool_t *pool)
{
	test_nodestore_update(pool);
	test_nodestore_protocol_select(pool);
}
