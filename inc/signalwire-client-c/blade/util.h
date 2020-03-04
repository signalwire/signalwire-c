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

KS_BEGIN_EXTERN_C

/**
* Given a method, will return the command flags appropriate for the method.
*/
static inline uint32_t BLADE_METHOD_FLAGS(const char * const method)
{
	ks_assertd(method != NULL);
	if (!strcmp(method, BLADE_BROADCAST_METHOD))
		return BLADE_BROADCAST_FLAGS;
	else if (!strcmp(method, BLADE_DISCONNECT_METHOD))
		return BLADE_DISCONNECT_FLAGS;
	else if (!strcmp(method, BLADE_NETCAST_METHOD))
		return BLADE_NETCAST_FLAGS;
	else if (!strcmp(method, BLADE_PROTOCOL_METHOD))
		return BLADE_PROTOCOL_FLAGS;
	else if (!strcmp(method, BLADE_IDENTITY_METHOD))
		return BLADE_IDENTITY_FLAGS;
	else if (!strcmp(method, BLADE_EXECUTE_METHOD))
		return BLADE_EXECUTE_FLAGS;
	else if (!strcmp(method, BLADE_SUBSCRIPTION_METHOD))
		return BLADE_SUBSCRIPTION_FLAGS;
	else if (!strcmp(method, BLADE_PING_METHOD))
		return BLADE_PING_FLAGS;
	else {
		ks_log(KS_LOG_WARNING, "Unsupported blade method: %s", method);
		return 0;
	}
}

KS_END_EXTERN_C
