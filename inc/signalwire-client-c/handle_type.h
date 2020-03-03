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

#define KS_BEGIN_EXTERN_C

/* Define the handle types in swclient. We start at the user group start
 * id, libks owns all groups prior. so we get 10 groups to play with
 * but for now we only use one, KS_HANDLE_GROUP_SWCLT. */

#define KS_HANDLE_GROUP_SWCLT KS_HANDLE_USER_GROUP_START

typedef enum {
	/* HMON - A handle monitoring context, used for state change
	 * detection on any swclient handle */
    SWCLT_HTYPE_HMON =  KS_HANDLE_MAKE_TYPE(SWCLT, 1),
} swclt_htype_t;

static inline ks_bool_t swclt_htype_valid(swclt_htype_t type)
{
	/* The type is valid if its group is our group */
	return KS_HANDLE_GROUP_FROM_TYPE(type) == KS_HANDLE_GROUP_SWCLT;
}

static inline const char *swclt_htype_str(swclt_htype_t type)
{
	switch(type) {
	case SWCLT_HTYPE_HMON:
		return "HandleMonitor";
	default:
		ks_abort_fmt("Invalid handle type: %lu", type);
	}
}

#define KS_END_EXTERN_C

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
