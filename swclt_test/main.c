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
#include "swclt_test.h"

DECLARE_TEST(json);
DECLARE_TEST(command);
DECLARE_TEST(execute);
DECLARE_TEST(connection);
DECLARE_TEST(session);
DECLARE_TEST(nodestore);
DECLARE_TEST(uncert_exp);

test_entry_t g_test_methods[] = {
	TEST_ENTRY(json),
	TEST_ENTRY(command),
	TEST_ENTRY(execute),
	TEST_ENTRY(connection),
	TEST_ENTRY(session),
	TEST_ENTRY(nodestore),
	TEST_ENTRY(uncert_exp),
};

static ks_spinlock_t g_log_lock;
const test_entry_t *g_current_test;
swclt_config_t *g_certified_config;
swclt_config_t *g_uncertified_config;
swclt_ident_t g_target_ident;
const char *g_target_ident_str;

static void __set_current_test(const test_entry_t *test_entry)
{
	ks_spinlock_acquire(&g_log_lock);
	g_current_test = test_entry;
	ks_spinlock_release(&g_log_lock);
}

static void __test_logger(const char *file, const char *func, int line, int level, const char *fmt, ...)
{
	va_list ap;
	char *data;
	static char log_line[1024];

	va_start(ap, fmt);

	ks_vasprintf(&data, fmt, ap);

	ks_spinlock_acquire(&g_log_lock);
	if (g_current_test)
		ks_snprintf(log_line, sizeof(log_line) - 2, "[TEST - %s] %s", g_current_test->name, data);
	else
		ks_snprintf(log_line, sizeof(log_line) - 2, "%s", data);

	if (log_line[strlen(log_line) - 1]  != '\n')
		strcat(log_line, "\n");

	printf("%s", log_line);
#if KS_PLAT_WIN
	OutputDebugStringA(log_line);
#endif
	fflush(stdout);

	free(data);

	ks_spinlock_release(&g_log_lock);

	va_end(ap);
}

void execute_test(const test_entry_t *entry)
{
	__set_current_test(entry);

	ks_pool_t *pool;
	REQUIRE(!ks_pool_open(&pool));

	entry->method(pool);

	REQUIRE(!ks_pool_close(&pool));

	__set_current_test(NULL);
}

void list_tests()
{
	printf("\n");
	for (int i = 0; i < sizeof(g_test_methods) / sizeof(test_entry_t); i++) {
		printf("   %s\n", g_test_methods[i].name);
	}
	printf("\n");
}

void execute_named_test(const char *name)
{
	for (int i = 0; i < sizeof(g_test_methods) / sizeof(test_entry_t); i++) {
		if (strcmp(name, g_test_methods[i].name))
			continue;
		execute_test(&g_test_methods[i]);
	}
}

void test_assertion(const char *assertion, const char *file, int line, const char *tag)
{
	ks_abort_fmt("Test: %s failed to assert: %s at: %s:%lu (%s)",
		g_current_test->name, assertion, file, line, tag);
}

int main(int argc, char **argv)
{
	ks_status_t status;
	ks_json_t *certified_config = NULL;
	ks_json_t *uncertified_config = NULL;

	if (argc == 2 && !strcmp(argv[1], "--list")) {
		list_tests();
		exit(0);
	}

	swclt_init(KS_LOG_LEVEL_DEBUG);
	ks_global_set_logger(__test_logger);

	certified_config = ks_json_create_object();
	uncertified_config = ks_json_create_object();

	ks_json_add_string_to_object(certified_config, "private_key_path", "./ca/intermediate/private/controller@freeswitch-upstream.key.pem");
	ks_json_add_string_to_object(certified_config, "client_cert_path", "./ca/intermediate/certs/controller@freeswitch-upstream.cert.pem");
	ks_json_add_string_to_object(certified_config, "cert_chain_path", "./ca/intermediate/certs/ca-chain.cert.pem");

	swclt_config_create(&g_certified_config);
	swclt_config_load_from_json(g_certified_config, certified_config);
	
	ks_json_add_string_to_object(uncertified_config, "authentication", "{ \"project\": \"06f784c6-6bd5-47fb-9897-407d66551333\", \"token\": \"PT2eddbccd77832e761d191513df8945d4e1bf70e8f3f74aaa\" }");

	swclt_config_create(&g_uncertified_config);
	swclt_config_load_from_json(g_uncertified_config, uncertified_config);

	ks_json_delete(&certified_config);
	ks_json_delete(&uncertified_config);

	g_target_ident_str = "blade://switchblade:2100";
	if (status = swclt_ident_from_str(&g_target_ident, NULL, g_target_ident_str)) {
		goto done;
	}
		
	if (argc > 1) {
		for (int test_selection = 1; test_selection < argc; test_selection++) {
			execute_named_test(argv[test_selection]);
		}
	} else {
		for (int i = 0; i < sizeof(g_test_methods) / sizeof(test_entry_t); i++) {
			const char *tail = strrchr(g_test_methods[i].name, '_');
			if (tail && !strcmp(tail, "_exp")) continue;
			
			execute_test(&g_test_methods[i]);
		}
	}

	ks_log(KS_LOG_INFO, "ALL TESTS PASS");

done:
	swclt_ident_destroy(&g_target_ident);
	swclt_config_destroy(&g_uncertified_config);
	swclt_config_destroy(&g_certified_config);

	if (swclt_shutdown()) {
		printf("WARNING Shutdown was ungraceful\n");
		ks_debug_break();
	}
	return status;
}
