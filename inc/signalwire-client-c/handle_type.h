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

#define KS_BEGIN_EXTERN_C

/* Define the handle types in swclient. We start at the user group start
 * id, libks owns all groups prior. so we get 10 groups to play with
 * but for now we only use one, KS_HANDLE_GROUP_SWCLT. */

#define KS_HANDLE_GROUP_SWCLT KS_HANDLE_USER_GROUP_START
#define KS_HANDLE_GROUP_SWCLT_SYS (KS_HANDLE_USER_GROUP_START + 1)

typedef enum {
	/* CONN - Connection which contains a transport
	 * to blade (e.g. websocket) handles reads and writes
	 * and knows how to do high level things like connect
	 * to blade with a connect request */
    SWCLT_HTYPE_CONN =  KS_HANDLE_MAKE_TYPE(SWCLT_SYS, 1),

	/* CMD - Commands are request/reply wrappers for json rpc */
    SWCLT_HTYPE_CMD =  KS_HANDLE_MAKE_TYPE(SWCLT_SYS, 2),

	/* FRAME - A frame is whats returned from a read on a transport,
	 * it provides transport specific attributes needed to handle
	 * a read or a write to a transport */
    SWCLT_HTYPE_FRAME =  KS_HANDLE_MAKE_TYPE(SWCLT_SYS, 3),

	/* WSS - Web socket transport which abstracts the reading
	 * and writing semantics of a web socket, including ssl
	 * negotiation, and ping/pong handling */
    SWCLT_HTYPE_WSS =  KS_HANDLE_MAKE_TYPE(SWCLT_SYS, 4),

	/* SESS	- A session is the highest level construct, and is the
	 * primary means in which a client ineracts with this sdk. */
    SWCLT_HTYPE_SESS =  KS_HANDLE_MAKE_TYPE(SWCLT_SYS, 5),

	/* SUB - A subscription holds the callback state for a subscription
	 * and is a child of a session. */
    SWCLT_HTYPE_SUB =  KS_HANDLE_MAKE_TYPE(SWCLT, 1),

	/* STORE - A node store contains the up to date state of
	 * provider/protocols/channels and routes  */
    SWCLT_HTYPE_STORE =  KS_HANDLE_MAKE_TYPE(SWCLT, 2),

	/* HMON - A handle monitoring context, used for state change
	 * detection on any swclient handle */
    SWCLT_HTYPE_HMON =  KS_HANDLE_MAKE_TYPE(SWCLT, 3),

	/* TEST - Used for unit testing */
    SWCLT_HTYPE_TEST =  KS_HANDLE_MAKE_TYPE(SWCLT, 4),
} swclt_htype_t;

static inline ks_bool_t swclt_htype_valid(swclt_htype_t type)
{
	/* The type is valid if its group is our group */
	return KS_HANDLE_GROUP_FROM_TYPE(type) == KS_HANDLE_GROUP_SWCLT ||
		KS_HANDLE_GROUP_FROM_TYPE(type) == KS_HANDLE_GROUP_SWCLT_SYS;
}

static inline const char *const swclt_htype_str(swclt_htype_t type)
{
	switch(type) {
	case SWCLT_HTYPE_CONN:
		return "Connection";
	case SWCLT_HTYPE_CMD:
		return "Command";
	case SWCLT_HTYPE_FRAME:
		return "Frame";
	case SWCLT_HTYPE_WSS:
		return "Websocket";
	case SWCLT_HTYPE_SESS:
		return "Session";
	case SWCLT_HTYPE_SUB:
		return "Subscription";
	case SWCLT_HTYPE_STORE:
		return "NodeStore";
	case SWCLT_HTYPE_HMON:
		return "HandleMonitor";
	case SWCLT_HTYPE_TEST:
		return "Test";
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
