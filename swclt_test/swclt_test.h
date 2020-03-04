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

/* Include signalwire-client-c */
#include "signalwire-client-c/client.h"

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define noreturn _Noreturn
#elif defined(__GNUC__)
#define noreturn __attribute__((noreturn))
#else
#define noreturn __declspec(noreturn)
#endif

extern swclt_config_t *g_certified_config;
extern swclt_config_t *g_uncertified_config;
extern swclt_ident_t g_target_ident;
extern const char *g_target_ident_str;

noreturn void test_assertion(const char *assertion, const char *file, int line, const char *tag);

#define REQUIRE(x)	\
	do { if (!(x)) test_assertion(#x, __FILE__, __LINE__, __PRETTY_FUNCTION__); } while (0)

#define FAIL(msg)	test_assertion(msg, __FILE__, __LINE__, __PRETTY_FUNCTION__)

typedef void (*test_method_t)(ks_pool_t *pool);

typedef struct test_entry_s {
	const char *name;
	test_method_t method;
} test_entry_t;

#define DECLARE_TEST(name) extern void test_##name(ks_pool_t *)
#define TEST_ENTRY(name) {#name, test_##name}

/* Load some utils */
#include "util/ssl.h"
