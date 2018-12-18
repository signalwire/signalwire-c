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

typedef enum {
	SWCLT_IDENT_TOSTRING_SCHEME = KS_BIT_FLAG(0),
	SWCLT_IDENT_TOSTRING_USER = KS_BIT_FLAG(1),
	SWCLT_IDENT_TOSTRING_HOST = KS_BIT_FLAG(2),
	SWCLT_IDENT_TOSTRING_PORT = KS_BIT_FLAG(3),
	SWCLT_IDENT_TOSTRING_PATH = KS_BIT_FLAG(4),
	SWCLT_IDENT_TOSTRING_PARAMETERS = KS_BIT_FLAG(5),
} swclt_ident_to_str_flags_t;

typedef struct swclt_ident_s {
	const char *uri;
	const char *components;
	const char *scheme;
	const char *user;
	const char *host;
	const char *port;
	const char *path;

	ks_port_t portnum;
	ks_hash_t *parameters;
} swclt_ident_t;

SWCLT_DECLARE(ks_status_t) swclt_ident_from_str(swclt_ident_t *ident, ks_pool_t *pool, const char *uri);
SWCLT_DECLARE(void) swclt_ident_to_str(swclt_ident_t *ident, ks_pool_t *pool, char **str, swclt_ident_to_str_flags_t flags);
SWCLT_DECLARE(void) swclt_ident_destroy(swclt_ident_t *ident);

KS_END_EXTERN_C
