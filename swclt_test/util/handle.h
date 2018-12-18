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

/* Load all internals for unit testing */
#include "signalwire-client-c/internal/command.h"
#include "signalwire-client-c/internal/connection.h"
#include "signalwire-client-c/internal/session.h"
#include "signalwire-client-c/internal/subscription.h"
#include "signalwire-client-c/transport/internal/frame.h"
#include "signalwire-client-c/transport/internal/websocket.h"

/* Create uber handle shortcuts for testing */
#define SWCLT_TEST_HANDLE_GET_DEF(name)										\
static inline swclt_##name##_ctx_t * name##_get(ks_handle_t handle)			\
{																			\
	swclt_##name##_ctx_t *ctx;												\
	REQUIRE(!swclt_##name##_get(handle, &ctx));								\
	return ctx;																\
}																			\
																			\
static inline void name##_put(swclt_##name##_ctx_t **ctx)					\
{																			\
	REQUIRE(!swclt_##name##_put(ctx));										\
}

SWCLT_TEST_HANDLE_GET_DEF(sess)
SWCLT_TEST_HANDLE_GET_DEF(conn)
SWCLT_TEST_HANDLE_GET_DEF(cmd)
SWCLT_TEST_HANDLE_GET_DEF(frame)
SWCLT_TEST_HANDLE_GET_DEF(wss)
SWCLT_TEST_HANDLE_GET_DEF(sub)

