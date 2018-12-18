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

typedef struct signalwire_messaging_send_response_s {
	const char *id;
	const char *status;
} signalwire_messaging_send_response_t;

SWCLT_JSON_MARSHAL_BEG(SIGNALWIRE_MESSAGING_SEND_RESPONSE, signalwire_messaging_send_response_t)
	SWCLT_JSON_MARSHAL_STRING(id)
	SWCLT_JSON_MARSHAL_STRING(status)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(SIGNALWIRE_MESSAGING_SEND_RESPONSE, signalwire_messaging_send_response_t)
	SWCLT_JSON_DESTROY_STRING(id)
	SWCLT_JSON_DESTROY_STRING(status)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(SIGNALWIRE_MESSAGING_SEND_RESPONSE, signalwire_messaging_send_response_t)
	SWCLT_JSON_PARSE_STRING(id)
	SWCLT_JSON_PARSE_STRING(status)
SWCLT_JSON_PARSE_END()

typedef struct signalwire_messaging_status_response_s {
	const char *id;
	const char *status;
} signalwire_messaging_status_response_t;

SWCLT_JSON_MARSHAL_BEG(SIGNALWIRE_MESSAGING_STATUS_RESPONSE, signalwire_messaging_status_response_t)
	SWCLT_JSON_MARSHAL_STRING(id)
	SWCLT_JSON_MARSHAL_STRING(status)
SWCLT_JSON_MARSHAL_END()

SWCLT_JSON_DESTROY_BEG(SIGNALWIRE_MESSAGING_STATUS_RESPONSE, signalwire_messaging_status_response_t)
	SWCLT_JSON_DESTROY_STRING(id)
	SWCLT_JSON_DESTROY_STRING(status)
SWCLT_JSON_DESTROY_END()

SWCLT_JSON_PARSE_BEG(SIGNALWIRE_MESSAGING_STATUS_RESPONSE, signalwire_messaging_status_response_t)
	SWCLT_JSON_PARSE_STRING(id)
	SWCLT_JSON_PARSE_STRING(status)
SWCLT_JSON_PARSE_END()
