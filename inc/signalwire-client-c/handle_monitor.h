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

/* Obfuscate our handle monitor internals */
typedef struct swclt_hmon_ctx swclt_hmon_ctx_t;

typedef ks_handle_t swclt_hmon_t;

/* Define the callback signature for the monitor */
typedef void(*swclt_hmon_state_change_cb_t)(ks_handle_t handle, const swclt_hstate_change_t *state_change_info, void *cb_data);

KS_BEGIN_EXTERN_C

SWCLT_DECLARE(ks_status_t) __swclt_hmon_register(
	swclt_hmon_t *hmon,
   	ks_handle_t handle_to_monitor,
   	swclt_hmon_state_change_cb_t cb,
   	void *cb_data,
   	const char *file,
   	int line,
   	const char *tag);

#define swclt_hmon_register(hmon, handle_to_monitor, cb, cb_data)	__swclt_hmon_register(	\
	hmon, handle_to_monitor, (swclt_hmon_state_change_cb_t)cb, cb_data, __FILE__, __LINE__, __PRETTY_FUNCTION__)

SWCLT_DECLARE(ks_status_t) swclt_hmon_raise_hstate_change(swclt_hmon_t, const swclt_hstate_change_t *state_change_info);

#define swclt_hmon_get(hmon, contextP)		__ks_handle_get(SWCLT_HTYPE_HMON, hmon, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_hmon_put(contextP)			__ks_handle_put(SWCLT_HTYPE_HMON, (ks_handle_base_t**)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)

KS_END_EXTERN_C
