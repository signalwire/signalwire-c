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

#include "signalwire-client-c/client.h"

/* The 'handle manager' in the signal wire client sdk, tracks all
 * sessions and connections every so often to service them
 * so that we can do things like re-establish dead sessions,
 * process any expired commands on connections, etc. */
static ks_thread_t *g_mgr_thread;				/* the manager thread context itself */
static ks_cond_t   *g_mgr_condition;			/* the manager sleeps on a condition for efficient wakeup */
static ks_status_t g_mgr_final_status;			/* We trap the last status here for some reason */
static ks_time_t   g_mgr_sleep_time_ms;			/* Our current sleep ms count */

#define now() 	(ks_time_ms(ks_time_now()))

static void __schedule_in_ms(ks_time_t schedule_in_ms)
{
	ks_cond_lock(g_mgr_condition);

	ks_assert(schedule_in_ms < 300000);

	if (!g_mgr_sleep_time_ms || g_mgr_sleep_time_ms > schedule_in_ms)
		ks_log(KS_LOG_DEBUG, "Waking manager in: %lu", schedule_in_ms);
	else {
		ks_log(KS_LOG_DEBUG, "Not signalling service for next service time of: %lu", schedule_in_ms);
		ks_log(KS_LOG_DEBUG, "Manager waking in: %lums", g_mgr_sleep_time_ms);
		ks_cond_unlock(g_mgr_condition);
		return;
	}

	g_mgr_sleep_time_ms = schedule_in_ms;

	ks_log(KS_LOG_DEBUG, "Manager next sleep duration is: %lums", g_mgr_sleep_time_ms);
	ks_cond_broadcast(g_mgr_condition);
	ks_cond_unlock(g_mgr_condition);
}

static void __report_state_change(swclt_hstate_listener_t *listener_ctx, swclt_handle_base_t *state_changed_ctx)
{
	swclt_handle_base_t *ctx;

	/* First we have to ensure the listener's handle is valid */
	if (ks_handle_get(0, listener_ctx->handle, &ctx))
		return;

	/* Finally, call the bastard */
	listener_ctx->cb(ctx, state_changed_ctx->pending_state_change_notification);

	ks_handle_put(0, &ctx);
}

static void __notify_monitor_children(swclt_handle_base_t *ctx)
{
	ks_handle_t next = KS_NULL_HANDLE;

	while (KS_STATUS_SUCCESS == ks_handle_enum_type(SWCLT_HTYPE_HMON, &next)) {
		ks_handle_t parent;

		if (ks_handle_parent(next, &parent))
			continue;

		/* If this monitor handle was watching the parent, raise its event */
		if (parent == ctx->handle) {
			ks_log(KS_LOG_DEBUG, "Manager raising event on child monitor: %s for handle: %s", ks_handle_describe_ctx((ks_handle_base_t *)ctx), ks_handle_describe(parent));

			swclt_hmon_raise_hstate_change(next, ctx->pending_state_change_notification);
		}
	}
}

static ks_bool_t __service_handle(ks_handle_t handle)
{
	swclt_handle_base_t *ctx;
	ks_bool_t serviced = KS_FALSE;

	if (ks_handle_get(0, handle, &ctx))
		return serviced;

	/* While we're here, handle state changes for the handle */
	ks_spinlock_acquire(&ctx->lock);

	ks_time_t now_time = now();

	if (!ctx->next_service_time_stamp_ms) {
		goto done;
	}

	if (now_time < ctx->next_service_time_stamp_ms) {
		/* Schedule it just to be sure */
		__schedule_in_ms(ctx->next_service_time_stamp_ms - now_time);
		goto done;
	}

	ks_log(KS_LOG_DEBUG, "Service begin: %s", ks_handle_describe_ctx(ctx));

	serviced = KS_TRUE;

	if (ctx->service_cb) {
		ks_spinlock_release(&ctx->lock);
		ctx->service_cb(ctx);
		ks_spinlock_acquire(&ctx->lock);
	}

	if (ctx->pending_state_change_notification) {
		ks_log(KS_LOG_DEBUG, "Handle: %s has pending state change notification", ks_handle_describe_ctx(ctx));

		/* Update the state now, before we notify, don't want to confuse people
		 * when the state is reported but the handle doesn't have the new state in place */
		ctx->state = ctx->pending_state_change_notification->new_state;

		for (uint32_t cb_idx = 0; cb_idx < ctx->c_state_listeners; cb_idx++) {
			swclt_hstate_listener_t state_listener;

			/* Grab the cb while under the lock then release it, we'll check if the
			 * array changed after we re-lock, this prevents some dicey scenarios where
			 * this lock gets held while a dependent thread is also locking on it */
			state_listener = ctx->state_listeners[cb_idx];

			ks_spinlock_release(&ctx->lock);

			/* Call the listener */
			__report_state_change(&state_listener, ctx);

			/* Great now, if the array changed, thats ok, it can only grow, so we can proceed */
			ks_spinlock_acquire(&ctx->lock);
		}

		/* Now iterate any monitors matching this parent and notify them too */
		ks_spinlock_release(&ctx->lock);
		__notify_monitor_children(ctx);
		ks_spinlock_acquire(&ctx->lock);

		/* State change is complete */
		ks_pool_free(&ctx->pending_state_change_notification);
	}

	/* Now see if the handle had a pending state change request w/cb set */
	if (ctx->pending_state_change_handler_cb) {
		ks_log(KS_LOG_DEBUG, "Handle: %s has pending state change", ks_handle_describe_ctx(ctx));

		/* Grab the info while holding the lock as it may change during the cb or while we call */
		swclt_hstate_state_change_handler_cb_t cb = ctx->pending_state_change_handler_cb;
		SWCLT_HSTATE new_state_request = ctx->pending_state_change_service;
		ks_status_t state_change_status;

		ctx->pending_state_change_service = SWCLT_HSTATE_INVALID;
		ctx->pending_state_change_handler_cb = NULL;

		/* Better be a non-invalid state */
		ks_assert(new_state_request != SWCLT_HSTATE_INVALID);

		/* Unlock and do the call */
		ks_spinlock_release(&ctx->lock);

		/* Call it, let it transition */
		if (state_change_status = cb(ctx, new_state_request)) {
			/* Re-queue for them */
			ks_log(KS_LOG_DEBUG, "State change attempt (%s=>%s) failed (%lu) re-queueing in: %lums",
					swclt_hstate_str(ctx->state), swclt_hstate_str(new_state_request), state_change_status, ctx->retry_service_delay_ms);

			swclt_hstate_initiate_change_in(ctx, new_state_request,
				cb, ctx->retry_service_delay_ms, ctx->retry_service_delay_ms);
		}

		ks_spinlock_acquire(&ctx->lock);

		/* If it was successful in its state change, go ahead and initiate a state change to the new
		 * state on behalf of the handle */
		if (state_change_status == KS_STATUS_SUCCESS) {
			ks_pool_free(&ctx->pending_state_change_notification);
			ks_spinlock_release(&ctx->lock);
			swclt_hstate_changed(ctx, new_state_request, KS_STATUS_SUCCESS, "Manager completed state change request");
			ks_spinlock_acquire(&ctx->lock);
		}

	}

	ks_log(KS_LOG_DEBUG, "Service end: %s", ks_handle_describe(handle));

done:
	ks_spinlock_release(&ctx->lock);
	ks_handle_put(0, &ctx);
	return serviced;
}

static void __service_handle_type(swclt_htype_t type)
{
	ks_handle_t next = KS_NULL_HANDLE;
	uint32_t total = 0;

	ks_log(KS_LOG_DEBUG, "Service manager enumerating handles of type: %s", swclt_htype_str(type));

	while (KS_STATUS_SUCCESS == ks_handle_enum_type(type, &next)) {
		if (ks_thread_stop_requested(g_mgr_thread)) {
			ks_log(KS_LOG_DEBUG, "Service manager stopping early due to stop request");
			break;
		}

		if (__service_handle(next))
			total++;
	}

	if (total)
		ks_log(KS_LOG_DEBUG, "Service manager serviced: %lu handles of type: %s", total, swclt_htype_str(type));
}

static void __service_handles()
{
	/* Now loop through all handles and service them */
	__service_handle_type(SWCLT_HTYPE_CONN);
    __service_handle_type(SWCLT_HTYPE_CMD);
    __service_handle_type(SWCLT_HTYPE_FRAME);
    __service_handle_type(SWCLT_HTYPE_WSS);
    __service_handle_type(SWCLT_HTYPE_SESS);
    __service_handle_type(SWCLT_HTYPE_SUB);
    __service_handle_type(SWCLT_HTYPE_STORE);
    __service_handle_type(SWCLT_HTYPE_TEST);
}

static ks_status_t __manager_loop()
{
	ks_log(KS_LOG_INFO, "Manager loop starting");

	/* Just hang out and wait to be woken up, with a hard wakeup at whatever
	 * the max is defined as no matter what */
	while (ks_thread_stop_requested(g_mgr_thread) == KS_FALSE) {
		ks_cond_lock(g_mgr_condition);

		if (g_mgr_sleep_time_ms) {
			ks_log(KS_LOG_DEBUG, "Manager sleeping for service in: %lums", g_mgr_sleep_time_ms);
			ks_cond_timedwait(g_mgr_condition, g_mgr_sleep_time_ms);
		}
		else {
			ks_log(KS_LOG_DEBUG, "Manager sleeping (not quite) indefinitely");
			ks_cond_timedwait(g_mgr_condition, 10000);
		}

		ks_log(KS_LOG_DEBUG, "Manager woke up, checking handles");

		/* Clear service time */
		g_mgr_sleep_time_ms = 0;

		ks_cond_unlock(g_mgr_condition);
		
		__service_handles();
	}

	ks_log(KS_LOG_INFO, "Manager loop stopping");

	return KS_STATUS_SUCCESS;
}

static void * __manager_thread_wrapper(ks_thread_t *thread, void *data)
{
	g_mgr_final_status = __manager_loop();
	return NULL;
}

SWCLT_DECLARE(ks_status_t) swclt_hmgr_init()
{
	ks_status_t status;

	if (status = ks_cond_create(&g_mgr_condition, NULL)) {
		ks_abort_fmt("Failed to allocate client manager condition: %lu", status);
	}

	if (status = ks_thread_create(&g_mgr_thread, __manager_thread_wrapper, NULL, NULL))
		ks_abort_fmt("Failed to allocate client manager thread: %lu", status);

	return status;
}

SWCLT_DECLARE(void) swclt_hmgr_shutdown()
{
	ks_log(KS_LOG_INFO, "Shutting down manager");

	/* Do this in the right order, lock,
	 * flag thread request stopped, unlock
	 * then join/destroy. */
	ks_cond_lock(g_mgr_condition);

	ks_thread_request_stop(g_mgr_thread);

	ks_cond_broadcast(g_mgr_condition);

	ks_cond_unlock(g_mgr_condition);

	ks_thread_destroy(&g_mgr_thread);
	ks_cond_destroy(&g_mgr_condition);
}

SWCLT_DECLARE(void) swclt_hmgr_request_service_now(swclt_handle_base_t *ctx)
{
	swclt_hmgr_request_service_in(ctx, 1);
}

SWCLT_DECLARE(void) swclt_hmgr_request_service_in(
	swclt_handle_base_t *ctx, ks_time_t next_service_ms)
{
	ks_log(KS_LOG_DEBUG, "Handle: %s is requesting manager service in: %lums",
		ks_handle_describe_ctx(ctx), next_service_ms);

	/* Wake up the manager at the given time by stashing this time then triggering it */
	ks_cond_lock(g_mgr_condition);

	/* Better be running */
	ks_assert(ks_thread_is_running(g_mgr_thread));

	/* Update our pending service time */
	ctx->next_service_time_stamp_ms = now() + next_service_ms;

	/* Request service */
	__schedule_in_ms(next_service_ms);

	ks_cond_unlock(g_mgr_condition);
}
