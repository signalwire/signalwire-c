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
#include "swclt_test.h"

#define now() (ks_time_ms(ks_time_now()))

/* Define our context */
typedef struct test_ctx_s {
	swclt_handle_base_t base;
	ks_time_t create_time;
	ks_time_t service_time;
	ks_time_t retry_time;
	ks_time_t retry_try_1_time;
	ks_cond_t *cond;
} test_ctx_t;

static ks_status_t __context_state_transition(test_ctx_t *ctx, SWCLT_HSTATE new_state)
{
	REQUIRE(new_state == SWCLT_HSTATE_ONLINE);

	if (ctx->service_time == 0) {
		/* Lets give it a max of 500ms for the assert */
		REQUIRE((now() - ctx->create_time) <= 500);
		ctx->service_time = now();

		/* Now fail the transition, should retry in ~1 seconds */
		return KS_STATUS_FAIL;
	} else if (ctx->retry_time == 0) {
		/* Should've servied us within 1.5 seconds */
		REQUIRE((now()  - ctx->service_time) <= 1500);

		ctx->retry_time = now();

		/* Fail it again */
		return KS_STATUS_FAIL;
	} else if (ctx->retry_try_1_time == 0) {

		/* Same check, should've servied us within 1.5 seconds */
		REQUIRE((now()  - ctx->retry_time) <= 1500);

		ctx->retry_try_1_time = now();

		/* Ok test complete, signal */
		ks_cond_lock(ctx->cond);
		ks_cond_broadcast(ctx->cond);
		ks_cond_unlock(ctx->cond);
		return KS_STATUS_SUCCESS;
	}

	FAIL("Should not have gotten here");
}

static ks_status_t __context_init(test_ctx_t *ctx, ks_cond_t *cond)
{
	ctx->cond = cond;

	REQUIRE(ctx->base.state == SWCLT_HSTATE_NORMAL);
	REQUIRE(ctx->base.last_state == SWCLT_HSTATE_NORMAL);

	ctx->create_time = now();

	/* Alright requet service right away */
	swclt_hstate_initiate_change_now(
		&ctx->base,
	   	SWCLT_HSTATE_ONLINE,
	   	__context_state_transition,
	   	1000);	/* retry duration */

	return KS_STATUS_SUCCESS;
}

static void __context_describe(test_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	snprintf(buffer, buffer_len, "Unit test yo");
}

static void __context_deinit(test_ctx_t *ctx)
{
}

static ks_status_t allocate(ks_handle_t *handle, ks_cond_t *cond)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M(
		NULL,
		SWCLT_HTYPE_TEST,
		handle,
		test_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		cond);
}

void test_hmanager(ks_pool_t *pool)
{
	ks_cond_t *cond;
	REQUIRE(!ks_cond_create(&cond, NULL));
	REQUIRE(!ks_cond_lock(cond));

	/* Create our own handle and test the signalwire client manager's */
	ks_handle_t handle;
	REQUIRE(!allocate(&handle, cond));

	/* Now wait for the test to end, should not take more hten 5 seconds, if so fail */
	if (ks_cond_timedwait(cond, 5000) == KS_STATUS_TIMEOUT)
		ks_abort("Test took too long");

	/* Success destroy it */
	ks_cond_unlock(cond);
	ks_handle_destroy(&handle);
	ks_cond_destroy(&cond);
}

