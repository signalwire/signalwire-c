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

typedef struct signalwire_provisioning_configure_response_s {
	ks_json_t *configuration;
} signalwire_provisioning_configure_response_t;

SWCLT_JSON_MARSHAL_BEG(SIGNALWIRE_PROVISIONING_CONFIGURE_RESPONSE, signalwire_provisioning_configure_response_t)
	SWCLT_JSON_MARSHAL_ITEM(configuration)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(SIGNALWIRE_PROVISIONING_CONFIGURE_RESPONSE, signalwire_provisioning_configure_response_t)
	SWCLT_JSON_DESTROY_ITEM(configuration)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(SIGNALWIRE_PROVISIONING_CONFIGURE_RESPONSE, signalwire_provisioning_configure_response_t)
	SWCLT_JSON_PARSE_ITEM(configuration)
SWCLT_JSON_PARSE_END()

