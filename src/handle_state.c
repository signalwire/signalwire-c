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

#define now() 	ks_time_ms(ks_time_now())

/* swclt_hstate_check_ctx - If the current state is degraded will return
 * the degraded reason code. It also takes an optional log message format
 * string. Note: The description for the state will be appended to the input string */
SWCLT_DECLARE(ks_status_t) swclt_hstate_check_ctx(const swclt_handle_base_t *ctx, const char *log_message)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	ks_spinlock_acquire(&ctx->lock);
	if (ctx->pending_state_change_notification) {
		status = ctx->pending_state_change_notification->status;

		if (log_message) {
			ks_log(KS_LOG_WARNING, "%s: %s", log_message, swclt_hstate_describe_change(ctx->pending_state_change_notification));
		}
	} else {
		switch (ctx->state) {
			case SWCLT_HSTATE_ONLINE:
			case SWCLT_HSTATE_NORMAL:
				break;

			case SWCLT_HSTATE_OFFLINE:
			case SWCLT_HSTATE_DEGRADED:
				status = KS_STATUS_NOT_ALLOWED;
				break;
		}
	}
	ks_spinlock_release(&ctx->lock);

	return status;
}

/* swclt_hstate_check - Given a handle, checks it out, then probes its pending state member
 * if set, returns the state, otherwise returns the normal state. Note: If the handle is invalid
 * we will assume a degraded state. */
SWCLT_DECLARE(ks_status_t) swclt_hstate_check(ks_handle_t handle, const char *log_message)
{
	swclt_handle_base_t *ctx;
	ks_status_t status;

	if (status = ks_handle_get(0, handle, &ctx)) {
		ks_log(KS_LOG_WARNING, "Unexpected degraded handle state on state check: %d", status);
		return status;
	}

	status = swclt_hstate_check_ctx(ctx, log_message);

	ks_handle_put(0, &ctx);

	return status;
}

/* swclt_handle_state - Returns the current state of the handles context, will
 * not wait for the service manager to service, once swclt_hstate_changed is
 * called, this api will immediatly return the new state. This is useful to
 * handle i/o into and out of handles, as you can first check the current state
 * before u do something. */
SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_ctx(swclt_handle_base_t *ctx)
{
	SWCLT_HSTATE state;

	ks_spinlock_acquire(&ctx->lock);
	if (ctx->pending_state_change_service)
		state = ctx->pending_state_change_service;
	else
		state = ctx->state;
	ks_spinlock_release(&ctx->lock);

	return state;
}

/* swclt_hstate_changed - Private function which can be called by any
 * client handle when they want to announce some kind of state change. The
 * state is set in the context and the manager is enqueued to service it. */
SWCLT_DECLARE(void) __swclt_hstate_changed(
	swclt_handle_base_t *ctx,
	SWCLT_HSTATE new_state,
	ks_status_t status,
	const char *reason,
	const char *file,
	int line,
	const char *tag)
{
	ks_spinlock_acquire(&ctx->lock);

	/* Layering is not allowed */
	if (ctx->pending_state_change_notification) {
		ks_log(KS_LOG_WARNING, "State change [%s=>%s] denied, pending state change present [%s=>%s]",
			ctx->state, new_state, ctx->pending_state_change_notification->new_state, ctx->pending_state_change_notification->new_state);

		ks_debug_break();
		ks_spinlock_release(&ctx->lock);
		return;
	}

	/* Set the state */
	ctx->last_state = ctx->state;
	ctx->state = new_state;

	if (!(ctx->pending_state_change_notification = ks_pool_alloc(ctx->pool, sizeof(swclt_hstate_change_t))))
		ks_abort("Failed to allocate bytes for state change request");

	ctx->pending_state_change_notification->status = status;
	ctx->pending_state_change_notification->new_state = new_state;
	ctx->pending_state_change_notification->old_state = ctx->last_state;

	if (!(ctx->pending_state_change_notification->reason = ks_pstrdup(ctx->pool, reason)))
		ks_abort("Failed to allocate bytes for state change request");

#if defined(KS_BUILD_DEBUG)
	ctx->pending_state_change_notification->file = file;
	ctx->pending_state_change_notification->line = line;
	ctx->pending_state_change_notification->tag = tag;
#endif

	if (ctx->pending_state_change_notification->new_state == SWCLT_HSTATE_DEGRADED || ctx->pending_state_change_notification->new_state == SWCLT_HSTATE_INVALID) {
#if defined(KS_BUILD_DEBUG)
		/* FORCE logging on from here on out */
		//swclt_enable_log_output(KS_LOG_LEVEL_DEBUG);
#endif
		ks_log(KS_LOG_WARNING, "Handle: %s state changed: %s for handle: %s", swclt_htype_str(KS_HANDLE_TYPE_FROM_HANDLE(ctx->handle)),
			swclt_hstate_describe_change(ctx->pending_state_change_notification), ks_handle_describe(ctx->handle));
	} else {
		ks_log(KS_LOG_INFO, "Handle: %s state changed: %s for handle: %s",
			swclt_htype_str(KS_HANDLE_TYPE_FROM_HANDLE(ctx->handle)), swclt_hstate_describe_change(ctx->pending_state_change_notification), ks_handle_describe(ctx->handle));
	}

	ks_spinlock_release(&ctx->lock);

	/* Request service real soon, but not so soon we freak out */
	swclt_hmgr_request_service_in(ctx, 1000);
}

/* swclt_hstate_register_listener - Adds a listener callback to a handles context. Listeners can be many to one, e.g. more then one
 * consumer may want to be called when say a websocket gets diconnected, this includes the connection
 * and/or the session, hence we have an allocated array of callback ptrs we register when this api
 * is called. The caller can optionally specify a handle we should verify for their context */
SWCLT_DECLARE(ks_status_t) __swclt_hstate_register_listener(
	swclt_handle_base_t *listening_ctx,
	swclt_hstate_change_cb_t state_cb,
	ks_handle_t state_source_handle,
	const char *file,
	int line,
	const char *tag)
{
	swclt_handle_base_t *state_source_ctx;

	/* We'll need to get to the context of the source the listener wants to watch */
	if (__ks_handle_get(0, state_source_handle, (ks_handle_base_t **)&state_source_ctx, file, line, tag)) {
		ks_log(KS_LOG_WARNING, "Error attempting to register a state change listener on invalid handle: %16.16llx for listener: %s",
			state_source_handle, ks_handle_describe(listening_ctx->handle));
		return KS_STATUS_HANDLE_INVALID;
	}

	/* Lock and register another listener on the source context */
	ks_spinlock_acquire(&state_source_ctx->lock);

	/* Re-alloc and grow the array by one callback */
	if (state_source_ctx->state_listeners) {
		if (!(state_source_ctx->state_listeners = ks_pool_resize(
				state_source_ctx->state_listeners, sizeof(swclt_hstate_listener_t) * state_source_ctx->c_state_listeners + 1)))
			ks_abort("Error re-allocating state change listener callback array");
	} else {
		if (!(state_source_ctx->state_listeners =
				ks_pool_alloc(state_source_ctx->pool, sizeof(swclt_hstate_listener_t))))
			ks_abort("Error allocating state change listener callback array");
	}

	/* And fill in the new entry */
	state_source_ctx->state_listeners[
		state_source_ctx->c_state_listeners].cb = state_cb;
	state_source_ctx->state_listeners[
		state_source_ctx->c_state_listeners].handle = listening_ctx->handle;
	state_source_ctx->c_state_listeners++;

	ks_spinlock_release(&state_source_ctx->lock);

	__ks_handle_put(0, (ks_handle_base_t **)&state_source_ctx, file, line, tag);
	return KS_STATUS_SUCCESS;
}

/* swclt_hstate_register_service - Binds a callback in the handles own context, that when set will
 * allow the client service to call into the handle periodically, when requested, to service it.
 * Servicing typically is how these handles do things like get re-connected when the socket dies. */
SWCLT_DECLARE(void) __swclt_hstate_register_service(
	swclt_handle_base_t *ctx,
	swclt_hstate_service_cb_t service_cb,
	const char *file,
	int line,
	const char *tag)
{
	ks_spinlock_acquire(&ctx->lock);
	ctx->service_cb = service_cb;
	ks_spinlock_release(&ctx->lock);
}

SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_get_ctx(const swclt_handle_base_t *ctx)
{
	SWCLT_HSTATE state;

	ks_spinlock_acquire(&ctx->lock);
	if (ctx->pending_state_change_service != SWCLT_HSTATE_INVALID)
		state = ctx->pending_state_change_service;
	else
		state = ctx->state;
	ks_assert(state != SWCLT_HSTATE_INVALID);
	ks_spinlock_release(&ctx->lock);

	return state;
}

/**
 * swclt_hstate_pending_get - Returns the pending state, useful for when you
 * want control during a poll of a state change.
 */
SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_pending_get(ks_handle_t handle)
{
	swclt_handle_base_t *ctx;
	SWCLT_HSTATE state;

	ks_assert(swclt_htype_valid(KS_HANDLE_TYPE_FROM_HANDLE(handle)));

	if(ks_handle_get(0, handle, &ctx))
		return SWCLT_HSTATE_INVALID;

	ks_spinlock_acquire(&ctx->lock);
	state = ctx->pending_state_change_service;
	ks_spinlock_release(&ctx->lock);

	ks_handle_put(0, &ctx);

	return state;
}

SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_current_get(ks_handle_t handle)
{
    swclt_handle_base_t *ctx;
    SWCLT_HSTATE state;

    ks_assert(swclt_htype_valid(KS_HANDLE_TYPE_FROM_HANDLE(handle)));

    if(ks_handle_get(0, handle, &ctx))
        return SWCLT_HSTATE_INVALID;

    ks_spinlock_acquire(&ctx->lock);
    state = ctx->state;
    ks_spinlock_release(&ctx->lock);

    ks_handle_put(0, &ctx);

    return state;
}

SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_get(ks_handle_t handle)
{
	swclt_handle_base_t *base;
	SWCLT_HSTATE state;

	ks_assert(swclt_htype_valid(KS_HANDLE_TYPE_FROM_HANDLE(handle)));

	if(ks_handle_get(0, handle, &base))
		return SWCLT_HSTATE_INVALID;

	state = swclt_hstate_get_ctx(base);

	ks_handle_put(0, &base);

	return state;
}

SWCLT_DECLARE(void) __swclt_hstate_initiate_change_now(
	swclt_handle_base_t *base,
	SWCLT_HSTATE new_state,
	swclt_hstate_state_change_handler_cb_t cb,
	ks_time_t retry_delay_ms)
{
	swclt_hstate_initiate_change_in(base, new_state, cb, 1, retry_delay_ms);
}

SWCLT_DECLARE(void) __swclt_hstate_initiate_change_in(
	swclt_handle_base_t *base,
	SWCLT_HSTATE new_state,
	swclt_hstate_state_change_handler_cb_t cb,
	ks_time_t next_service_ms,
	ks_time_t retry_delay_ms)
{
	ks_log(KS_LOG_INFO, "Handle: %s is initiating state change (state change: %s=>%s) in: %lums",
			ks_handle_describe_ctx(base), swclt_hstate_str(base->state), swclt_hstate_str(new_state), next_service_ms);

	/* Update our pending service flag to true */
	ks_spinlock_acquire(&base->lock);
	ks_assert(base->state != new_state);
	base->pending_state_change_service = new_state;
	base->pending_state_change_handler_cb = cb;
	base->next_service_time_stamp_ms = now() + next_service_ms;
	base->retry_service_delay_ms = retry_delay_ms;
	ks_spinlock_release(&base->lock);

	swclt_hmgr_request_service_in(base, next_service_ms);
}
