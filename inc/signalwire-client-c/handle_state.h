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

#pragma once

/* Handle states provide hooks to do things when they change, and they
 * describe whether the handle is open for use or not */
typedef enum {
	SWCLT_HSTATE_INVALID,	/* this is the no state, only used to distinguish between set and not set */
	SWCLT_HSTATE_NORMAL,	/* the default state, we assume everything is normal post allocation. */
	SWCLT_HSTATE_ONLINE,	/* the handle is now 'online' in whatever definition they want to consider it to be */
	SWCLT_HSTATE_OFFLINE,	/* the handle is now 'offline' in whatever definition they want to consider it to be */
	SWCLT_HSTATE_DEGRADED,	/* the service provided by the handle has halted due to some kind of failure.
							 * Note: The primary difference between DEGRADED and OFFLINE is that DEGRADED is
							 * caused by some kind of error, wherease OFFLINE may just be a request from the client. */
} SWCLT_HSTATE;

/* swclt_hstate_str - Returns a string version for the state enumeration value */
static inline const char *swclt_hstate_str(SWCLT_HSTATE state)
{
	switch (state) {
		case SWCLT_HSTATE_INVALID:
			return "Invalid";
		case SWCLT_HSTATE_NORMAL:
			return "Normal";
		case SWCLT_HSTATE_ONLINE:
			return "Online";
		case SWCLT_HSTATE_OFFLINE:
			return "Offline";
		case SWCLT_HSTATE_DEGRADED:
			return "Degraded";
		default:
			ks_abort_fmt("Invalid handle state: %d", state);
	}
}

/* Forward declare our handle type as it is composed of inner types that reference it */
typedef struct swclt_hstate_change_request swclt_hstate_change_t;
typedef struct swclt_handle_base swclt_handle_base_t;

/* A state change callback is available on any signalwire client handle */
typedef void(*swclt_hstate_change_cb_t)(swclt_handle_base_t *ctx, swclt_hstate_change_t *state_change_request);

/* State change handler for when a handle wants to transition state and execute code as part of the transition */
typedef ks_status_t (*swclt_hstate_state_change_handler_cb_t)(swclt_handle_base_t *ctx, SWCLT_HSTATE requested_new_state);

/* Service callback for all client handles */
typedef void(*swclt_hstate_service_cb_t)(swclt_handle_base_t *ctx);

/* Context structure used to describe a state change listener on a handle */
typedef struct swclt_hstate_listener_s {
	swclt_hstate_change_cb_t cb;		/* Called anytime state changes on this handle, by the service manager thread */
	ks_handle_t handle;						/* We check this ptr out before calling the callers callback */
} swclt_hstate_listener_t;

/* The state change request structure gets allocated by the handle anytime it wants to start a state change
 * request. The manager will then take this state change and apply it in its thread */
struct swclt_hstate_change_request {
	/* the requested state to change to */
	SWCLT_HSTATE new_state;

	/* the original state snapshotted at allocation */
	SWCLT_HSTATE old_state;

	/* Reason and status for the state change (reason must be non zero when
	 * state is degraded, and must be zero when state is normal) */
	char *reason;
	ks_status_t status;

#if defined(KS_BUILD_DEBUG)
	/* For debugging we stash the file/line/tag anytime state changes */
	const char *file;
	const char *tag;
	int line;
#endif
};

/* Describes in detail the pending state change, including the reason and status code. If debug
 * is enabled, will include the file/line where the state change was initiated from. */
static inline const char *swclt_hstate_describe_change(const swclt_hstate_change_t *pending_state_change)
{
	static KS_THREAD_LOCAL char buf[1024] = { 0 };
#if defined(KS_BUILD_DEBUG)
	snprintf(buf, sizeof(buf), "[%s=>%s] Status: %d Reason: %s\nLocation: %s:%d:%s",
		swclt_hstate_str(pending_state_change->old_state),
		swclt_hstate_str(pending_state_change->new_state),
		pending_state_change->status, pending_state_change->reason,
		pending_state_change->file, pending_state_change->line, pending_state_change->tag);
#else
	snprintf(buf, sizeof(buf), "[%s=>%s] Status: %d Reason: %s",
		swclt_hstate_str(pending_state_change->old_state),
		swclt_hstate_str(pending_state_change->new_state),
		pending_state_change->status, pending_state_change->reason);
#endif
	return buf;
}

/* Private handle state apis used internally, but still exposed for unit tests */
SWCLT_DECLARE(ks_status_t) swclt_hstate_check_ctx(const swclt_handle_base_t *ctx, const char *log_message);
SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_get_ctx(const swclt_handle_base_t *ctx);
SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_get(ks_handle_t handle);
SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_pending_get(ks_handle_t handle);
SWCLT_DECLARE(SWCLT_HSTATE) swclt_hstate_current_get(ks_handle_t handle);
SWCLT_DECLARE(ks_status_t) swclt_hstate_check(ks_handle_t handle, const char *log_message);

SWCLT_DECLARE(void) __swclt_hstate_changed(swclt_handle_base_t *ctx,SWCLT_HSTATE new_state, ks_status_t status, const char *reason, const char *file, int line, const char *tag);

SWCLT_DECLARE(void) __swclt_hstate_initiate_change_in(
	swclt_handle_base_t *base,
	SWCLT_HSTATE new_state,
	swclt_hstate_state_change_handler_cb_t cb,
	ks_time_t next_service_ms,
	ks_time_t retry_delay_ms);
#define swclt_hstate_initiate_change_in(base, new_state, cb, next_service_ms, retry_delay_ms)\
   	__swclt_hstate_initiate_change_in(base, new_state, (swclt_hstate_state_change_handler_cb_t)cb, next_service_ms, retry_delay_ms)

SWCLT_DECLARE(void) __swclt_hstate_initiate_change_now(
	swclt_handle_base_t *base,
	SWCLT_HSTATE new_state,
	swclt_hstate_state_change_handler_cb_t cb,
	ks_time_t retry_delay_ms);

#define swclt_hstate_initiate_change_now(base, new_state, cb, retry_delay_ms)\
   	__swclt_hstate_initiate_change_now(base, new_state, (swclt_hstate_state_change_handler_cb_t)cb, retry_delay_ms)

SWCLT_DECLARE(ks_status_t) __swclt_hstate_register_listener(
	swclt_handle_base_t *listening_ctx,
   	swclt_hstate_change_cb_t state_change_cb,
   	ks_handle_t state_change_source_handle,
   	const char *file,
   	int line,
   	const char *tag);

SWCLT_DECLARE(void) __swclt_hstate_register_service(swclt_handle_base_t *ctx, swclt_hstate_service_cb_t service_cb, const char *file, int line, const char *tag);

/* Use these macros to forward the context to debug build structures while also providing a cast for
 * the callbacks. */
#define swclt_hstate_changed(ctx, new_state, status, reason) __swclt_hstate_changed(ctx, new_state, status, reason, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_hstate_register_listener(ctx, cb, handle) __swclt_hstate_register_listener((swclt_handle_base_t *)ctx, (swclt_hstate_change_cb_t)cb, handle, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_hstate_register_service(ctx, service_cb) __swclt_hstate_register_service(ctx, (swclt_hstate_service_cb_t)service_cb, __FILE__, __LINE__, __PRETTY_FUNCTION__)
