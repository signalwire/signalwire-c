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

/* Define our base handle structure, this is used to generically provide features
 * across all client handle types */
struct swclt_handle_base {
	/* Just so we don't have to have another address on dereference
	 * manually define the same entries used in ks_handle_base_t */
	ks_handle_t handle;
	ks_pool_t *pool;

	/* This is the current handle state */
	SWCLT_HSTATE state;

	/* The last state we transitioned from */
	SWCLT_HSTATE last_state;

	/* Set to the state the handle is requesting a service for, this is different
	 * then the swclt_hstate_change_t system, that system is for reporting, this one
	 * is used by the reporting handle itself when it wants to transition state */
	volatile SWCLT_HSTATE pending_state_change_service;
	volatile swclt_hstate_state_change_handler_cb_t pending_state_change_handler_cb;

	/* Lightweight lock for managing the callback array */
	ks_spinlock_t lock;

	/* Service callback, if set makes the handle available for service event from
	 * the managers thread */
	swclt_hstate_service_cb_t service_cb;

	/* This is set by the handle when they want service, its used to contextualize
	 * the next requested service absolute timestamp, so that we do not service the handle
	 * sooner then it needs, nor do we service handles which have not requested
	 * a service time */
	ks_time_t next_service_time_stamp_ms;

	/* If we fail to service, this is the delay in ms for the next attempt */
	ks_time_t retry_service_delay_ms;

	/* When non null, this implies a state change is pending, and it contains
	 * the information needed to articulate why and what for the state change
	 * Note: You cannot layer state change requests, multiple layered requests to
	 * change state will be ignored. */
	swclt_hstate_change_t *pending_state_change_notification;

	/* State change gets raised by the handle anytime its state changes. The
	 * definition of a state change is relative to the type as the states are
	 * generic. These contextes allow for a one to many relationship between
	 * handles listening for a state change, and a handle reporting a
	 * state change. */
	swclt_hstate_listener_t *state_listeners;
	uint32_t c_state_listeners;
};

#define SWCLT_HANDLE_ALLOC_TEMPLATE_S_TAG(pool, file, line, tag, handle_type, handle_ptr, context_type, initial_hstate, describe_method, deinit_method, init_method)	\
	ks_status_t status;																									\
	context_type *context;																								\
																														\
	if ((status = __ks_handle_alloc_ex(pool, handle_type, sizeof(context_type),											\
				(ks_handle_base_t **)&context, handle_ptr,																\
				(ks_handle_describe_cb_t)describe_method,																\
				(ks_handle_deinit_cb_t)deinit_method,																	\
				file, line, tag))) {										 											\
		if (status == KS_STATUS_HANDLE_NO_MORE_SLOTS) {																		\
			abort();																									\
		}																												\
		return status;																									\
	}																													\
																														\
	((swclt_handle_base_t *)context)->state = initial_hstate;															\
	((swclt_handle_base_t *)context)->last_state = initial_hstate;														\
																														\
	if (status = init_method(context)) {																				\
		deinit_method(context);																							\
		ks_handle_destroy(handle_ptr);																					\
		return status;																									\
	}																													\
																														\
	ks_handle_set_ready(*handle_ptr);																					\
	return status;

#define SWCLT_HANDLE_ALLOC_TEMPLATE_M_TAG(pool, file, line, tag, handle_type, handle_ptr, context_type, initial_hstate, describe_method, deinit_method, init_method, ...)		\
	ks_status_t status;																											\
	context_type *context;																										\
																																\
	if ((status = __ks_handle_alloc_ex(pool, handle_type, sizeof(context_type), (ks_handle_base_t **)&context, handle_ptr,		\
				(ks_handle_describe_cb_t)describe_method, (ks_handle_deinit_cb_t)deinit_method,									\
				file, line, tag))) {								 															\
		if (status == KS_STATUS_HANDLE_NO_MORE_SLOTS) {																			\
			abort();																											\
		}																														\
		return status;																											\
	}																															\
																																\
	((swclt_handle_base_t *)context)->state = initial_hstate;																	\
	((swclt_handle_base_t *)context)->last_state = initial_hstate;																\
																																\
	if (status = init_method(context, __VA_ARGS__)) {																			\
		deinit_method(context);																									\
		ks_handle_destroy(handle_ptr);																							\
		return status;																											\
	}																															\
																																\
	ks_handle_set_ready(*handle_ptr);																							\
	return status;

#define SWCLT_HANDLE_ALLOC_TEMPLATE_S(pool, handle_type, handle_ptr, context_type, initial_hstate, describe_method, deinit_method, init_method)	\
	ks_status_t status;																									\
	context_type *context;																								\
																														\
	if ((status = ks_handle_alloc_ex(pool, handle_type, sizeof(context_type),											\
				&context, handle_ptr,																					\
				(ks_handle_describe_cb_t)describe_method,																\
				(ks_handle_deinit_cb_t)deinit_method))) {																\
		if (status == KS_STATUS_HANDLE_NO_MORE_SLOTS) {																	\
			abort();																									\
		}																												\
		return status;																									\
	}																													\
																														\
	((swclt_handle_base_t *)context)->state = initial_hstate;															\
	((swclt_handle_base_t *)context)->last_state = initial_hstate;														\
																														\
	if (status = init_method(context)) {																				\
		deinit_method(context);																							\
		ks_handle_destroy(handle_ptr);																					\
		return status;																									\
	}																													\
																														\
	ks_handle_set_ready(*handle_ptr);																					\
	return status;

#define SWCLT_HANDLE_ALLOC_TEMPLATE_M(pool, handle_type, handle_ptr, context_type, initial_hstate, describe_method, deinit_method, init_method, ...)			\
	ks_status_t status;																												\
	context_type *context;																											\
																																	\
	if ((status = ks_handle_alloc_ex(pool, handle_type, sizeof(context_type), &context, handle_ptr,									\
				(ks_handle_describe_cb_t)describe_method, (ks_handle_deinit_cb_t)deinit_method))) {									\
		if (status == KS_STATUS_HANDLE_NO_MORE_SLOTS) {																				\
			abort();																												\
		}																															\
		return status;																												\
	}																																\
																																	\
	((swclt_handle_base_t *)context)->state = initial_hstate;																		\
	((swclt_handle_base_t *)context)->last_state = initial_hstate;																	\
																																	\
	if (status = init_method(context, __VA_ARGS__)) {																				\
		deinit_method(context);																										\
		ks_handle_destroy(handle_ptr);																								\
		return status;																												\
	}																																\
																																	\
	ks_handle_set_ready(*handle_ptr);																								\
	return status;


KS_END_EXTERN_C
