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

#include "signalwire-client-c/client.h"
#if 0

#define REQUIRE(x) 	ks_assertd(x)

static swclt_sess_t g_session;
static const char *g_bench_protocol = "bench";
static const char *g_bench_channel = "swbench";
static const char *g_bench_event = "report";
static ks_status_t g_bench_thread_status;
static ks_throughput_t g_throughput_send;
static ks_throughput_t g_throughput_recv;

#define TEST_DURATION_SEC	50000000

/* Define our properties for this benchmark */
static SSL_CTX * __create_ssl_ctx()
{
	static const char *key = "ca/intermediate/private/client@freeswitch-upstream.key.pem";
	static const char *cert = "ca/intermediate/certs/client@freeswitch-upstream.cert.pem";
	static const char *chain = "ca/intermediate/certs/ca-chain.cert.pem";
	SSL_CTX *ssl;

	REQUIRE(!swclt_ssl_create_context(key, cert, chain, &ssl));
	return ssl;
}

void render_stats(const char *header)
{
	printf("%s\n", header);
	printf("Send: %s\n", ks_handle_describe(g_throughput_send));
	printf("Recv: %s\n", ks_handle_describe(g_throughput_recv));
}

void on_bench_event(
	swclt_sess_t sess,
	blade_broadcast_rqu_t *rqu,
	void *cb_data)
{
	ks_json_t *object = BLADE_BROADCAST_RQU_MARSHAL(NULL, *rqu);

	KS_JSON_PRINT("Got bench event:", object);

	ks_json_delete(&object);
}

SWCLT_JSON_MARSHAL_BEG(THROUGHPUT_STATS, ks_throughput_stats_t)
	SWCLT_JSON_MARSHAL_INT(size)
	SWCLT_JSON_MARSHAL_INT(count)
	SWCLT_JSON_MARSHAL_DOUBLE(rate_size)
	SWCLT_JSON_MARSHAL_DOUBLE(rate_count)
	SWCLT_JSON_MARSHAL_INT_EX(run_time_sec, run_time)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(THROUGHPUT_STATS, ks_throughput_stats_t)
	SWCLT_JSON_DESTROY_INT(size)
	SWCLT_JSON_DESTROY_INT(count)
	SWCLT_JSON_DESTROY_DOUBLE(rate_size)
	SWCLT_JSON_DESTROY_DOUBLE(rate_count)
	SWCLT_JSON_DESTROY_INT_EX(run_time_sec, run_time)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(THROUGHPUT_STATS, ks_throughput_stats_t)
	SWCLT_JSON_PARSE_INT(size)
	SWCLT_JSON_PARSE_INT(count)
	SWCLT_JSON_PARSE_DOUBLE(rate_size)
	SWCLT_JSON_PARSE_DOUBLE(rate_count)
	SWCLT_JSON_PARSE_INT_EX(run_time_sec, run_time)
SWCLT_JSON_PARSE_END()

/* Define our benchmark payloads */
typedef struct benchmark_payload_s {
	ks_throughput_stats_t *conn_recv_stats;
	ks_throughput_stats_t *conn_send_stats;
} benchmark_payload_t;

SWCLT_JSON_MARSHAL_BEG(BENCHMARK_PAYLOAD, benchmark_payload_t)
	SWCLT_JSON_MARSHAL_CUSTOM(THROUGHPUT_STATS, conn_recv_stats)
	SWCLT_JSON_MARSHAL_CUSTOM(THROUGHPUT_STATS, conn_send_stats)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(BENCHMARK_PAYLOAD, benchmark_payload_t)
	SWCLT_JSON_DESTROY_CUSTOM(THROUGHPUT_STATS, conn_recv_stats)
	SWCLT_JSON_DESTROY_CUSTOM(THROUGHPUT_STATS, conn_send_stats)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(BENCHMARK_PAYLOAD, benchmark_payload_t)
	SWCLT_JSON_PARSE_CUSTOM(THROUGHPUT_STATS, conn_recv_stats)
	SWCLT_JSON_PARSE_CUSTOM(THROUGHPUT_STATS, conn_send_stats)
SWCLT_JSON_PARSE_END()

ks_json_t * __create_bench_payload()
{
	ks_throughput_stats_t recv, send;
	benchmark_payload_t payload = {&recv, &send};

	REQUIRE(!ks_throughput_stats(g_throughput_recv, &recv));
	REQUIRE(!ks_throughput_stats(g_throughput_send, &send));

	return BENCHMARK_PAYLOAD_MARSHAL(NULL, payload);
}

void * bench_thread_run(ks_thread_t *thread, void *data)
{
	ks_time_t start = ks_time_now_sec();
	ks_time_t last_update = start;

	printf("Bench thread is active\n");

	while (!ks_thread_stop_requested(thread)) {
		ks_json_t *bench_payload = __create_bench_payload();

		if (g_bench_thread_status = swclt_sess_broadcast(
				g_session,
				g_bench_protocol,
				g_bench_channel,
				g_bench_event,
				bench_payload
				))	{
			ks_json_delete(&bench_payload);
			printf("Bench errored on broadcast: %d\n", g_bench_thread_status);
			break;
		}

		ks_json_delete(&bench_payload);

		if (ks_time_now_sec() - last_update > 1) {
			render_stats("Stats");
			last_update = ks_time_now_sec();
		}

		if ((ks_time_now_sec() - start) >= TEST_DURATION_SEC)
			break;
	}

	printf("Bench thread exiting with final status: %d\n", g_bench_thread_status);

	return NULL;
}

ks_status_t do_bench()
{
	ks_thread_t *bench_thread = NULL;
	ks_status_t status;

	/* Start a bench thread */
	if (status = ks_thread_create(&bench_thread, bench_thread_run, NULL, NULL))
		return status;

	/* And let it do its thing */
	ks_thread_join(bench_thread);
	ks_thread_destroy(&bench_thread);
	return g_bench_thread_status;
}

int main(int argc, char **argv)
{
	static const char *key = "ca/intermediate/private/client@freeswitch-upstream.key.pem";
	static const char *cert = "ca/intermediate/certs/client@freeswitch-upstream.cert.pem";
	static const char *chain = "ca/intermediate/certs/ca-chain.cert.pem";
	swclt_sub_t sub;
	ks_status_t status;
	const char *identity;
	swclt_cfg_t swclt_cfg, bench_cfg;
	swclt_sess_info_t info;

	if (status = swclt_init(KS_LOG_LEVEL_DEBUG))
		return status;

	if (status = swclt_cfg_open_ex(&bench_cfg, "swclt_bench.cfg", "benchmark"))
		goto done;
	if (status = swclt_cfg_open_ex(&swclt_cfg, "swclt_bench.cfg", "signalwire_client"))
		goto done;

	if (status = swclt_cfg_get_strval(bench_cfg, "target_identity", &identity))
		goto done;

	printf("swclt_bench starting, connecting to %s\n", identity);

	if (status = swclt_sess_create(&g_session, identity, swclt_cfg))
		goto done;

	/* Watch the session for state changes, anytime it goes normal we'll
	 * start pumping the benchmark payloads */
	if (status = swclt_handle_register_state_change_listener(

#define swclt_handle_register_state_change_listener(ctx, cb, handle) __swclt_handle_register_state_change_listener((swclt_handle_base_t *)ctx, (swclt_hstate_change_cb_t)cb, handle)

	/* Initiate the connect phase */
	if (status = swclt_sess_connect(g_session))
		goto done;

	if (status = swclt_sess_info(g_session, &info)) {
		printf("Failed to get session info: %d\n", status);
		goto done;
	}

	printf("Successfully allocated session: %s\n", ks_handle_describe(g_session));

	/* Grab a copy of the rate reporters so we can report our rate */
	swclt_sess_get_rates(g_session, &g_throughput_recv, &g_throughput_send);

	printf("Publishing bench channel\n");

	/* Publish a bench channel */
	if (status = swclt_sess_provider_add(g_session, g_bench_protocol,
			(blade_channel_t){g_bench_channel, BLADE_ACL_PUBLIC},
			BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC, BLADE_ACL_PUBLIC)) {
		printf("Failed to publish bench protocol: %d\n", status);
		goto done;
	}

	printf("Successfully published bench channel, subscribing next...");

	/* And subscribe to it */
	if (status = swclt_sess_subscription_add(
			g_session,
			g_bench_protocol,
			g_bench_channel,
			&on_bench_event,
			NULL,
			&sub)) {
		printf("Failed to subscribe to bench channel: %d", status);
		goto done;
	}

	printf("Successfully subscribed to bench channel, starting bench thread");
;
	/* Kick off the bench loop */
	if (status = do_bench()) {
		printf("Bench failed: %d", status);
		goto done;
	}

	ks_throughput_stop(g_throughput_send);
	ks_throughput_stop(g_throughput_recv);

	render_stats("Final stats");

done:
	ks_handle_destroy(&g_session);
	swclt_shutdown();

	return status;
}

#endif

int main(int argc, char **argv)
{
	/** @@ TODo **/
	return -1;
}
