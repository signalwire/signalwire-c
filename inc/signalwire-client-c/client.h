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

#include "libks/ks.h"
#include "libks/ks_atomic.h"

#include "signalwire-client-c/export.h"
#include "signalwire-client-c/init.h"
#include "signalwire-client-c/transport/frame.h"
#include "signalwire-client-c/command.h"
#include "signalwire-client-c/transport/websocket.h"
#include "signalwire-client-c/JSON/macros.h"
#include "signalwire-client-c/blade/blade.h"
#include "signalwire-client-c/signalwire/signalwire.h"
#include "signalwire-client-c/identity.h"
#include "signalwire-client-c/config.h"
#include "signalwire-client-c/connection.h"
#include "signalwire-client-c/subscription.h"
#include "signalwire-client-c/pmethod.h"
#include "signalwire-client-c/nodestore.h"
#include "signalwire-client-c/session.h"
#include "signalwire-client-c/version.h"

/* Utility */
#include "signalwire-client-c/ssl.h"


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
