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

#define SWCLT_BLADE_VERSION_MAJOR		2
#define SWCLT_BLADE_VERSION_MINOR		4
#define SWCLT_BLADE_VERSION_REVISION	1

/* Define the default time to live amount for all blade commands */
#define BLADE_DEFAULT_CMD_TTL_MS		10000

#include "signalwire-client-c/blade/type.h"
#include "signalwire-client-c/blade/connect.h"
#include "signalwire-client-c/blade/disconnect.h"
#include "signalwire-client-c/blade/authenticate.h"
#include "signalwire-client-c/blade/authority.h"
#include "signalwire-client-c/blade/broadcast.h"
#include "signalwire-client-c/blade/execute.h"
#include "signalwire-client-c/blade/identity.h"
#include "signalwire-client-c/blade/ping.h"
#include "signalwire-client-c/blade/protocol.h"
#include "signalwire-client-c/blade/register.h"
#include "signalwire-client-c/blade/subscription.h"
#include "signalwire-client-c/blade/netcast.h"
#include "signalwire-client-c/blade/util.h"

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
