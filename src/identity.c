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

#include "signalwire-client-c/client.h"

SWCLT_DECLARE(ks_status_t) swclt_ident_from_str(swclt_ident_t *ident, ks_pool_t *pool, const char *uri)
{
	ks_status_t ret = KS_STATUS_SUCCESS;
	char *tmp = NULL;
	char *tmp2 = NULL;
	char terminator = '\0';

	ks_assert(ident && uri);

	ks_log(KS_LOG_DEBUG, "Parsing %s\n", uri);

	swclt_ident_destroy(ident);

	ident->uri = ks_pstrdup(pool, uri);
	ident->components = tmp = ks_pstrdup(pool, uri);

	// Supported components with pseudo regex
	// [scheme://] [user@] <host> [:port] [/path] [;param1=value1] [;param2=value2]

	// scheme is optional, but must have at least '://' suffix to qualify as the scheme
	if ((tmp2 = strchr(tmp, ':'))) {
		if (*(tmp2 + 1) == '/' && *(tmp2 + 2) == '/') {
			ident->scheme = tmp;
			*tmp2 = '\0';
			tmp2 += 3;
			tmp = tmp2;
		}
		else if (*(tmp2 + 1) == '/') {
			ret = KS_STATUS_FAIL;
			goto done;
		}

		if (!*tmp) {
			ret = KS_STATUS_FAIL;
			goto done;
		}
	}

	// if found, null terminate scheme portion
	// may have trailing '/' characters which are optional, this is not perfect it should probably only match a count of 0 or 2 explicitly
	// must have more data to define at least a host

	// next component may be the user or the host, so it may start with optional: <user>@
	// or it may skip to the host, which may be terminated by an optional port, optional path, optional parameters, or the end of the uri
	// which means checking if an '@' appears before the next ':' for port, '/' for path, or ';' for parameters, if @ appears at all then it appears before end of the uri
	// @todo need to account for host being encapsulated by '[' and ']' as in the case of an IPV6 host to distinguish from the port, but for simplicity allow any
	// host to be encapsulated in which case if the string starts with a '[' here, then it MUST be the host and it MUST be terminated with the matching ']' rather than other
	// optional component terminators
	if (!(tmp2 = strpbrk(tmp, "@:/;"))) {
		// none of the terminators are found, treat the remaining string as a host
		ident->host = tmp;
		goto done;
	}

	// grab the terminator and null terminate for the next component
	terminator = *tmp2;
	*tmp2++ = '\0';

	if (terminator == '@') {
		// if the terminator was an '@', then we have a user component before the host
		ident->user = tmp;

		tmp = tmp2;

		if (!(*ident->user)) {
			ret = KS_STATUS_FAIL;
			goto done;
		}

		// repeat the same as above, except without looking for '@', to find only the end of the host, parsing to the same point as above if user was not found
		if (!(tmp2 = strpbrk(tmp, ":/;"))) {
			// none of the terminators are found, treat the remaining string as a host
			ident->host = tmp;
			goto done;
		}

		// grab the terminator and null terminate for the next component
		terminator = *tmp2;
		*tmp2++ = '\0';
	}

	// at this point the user portion has been parsed if it exists, the host portion has been terminated, and there is still data remaining to parse
	// @todo need to account for host being encapsulated by '[' and ']' as in the case of an IPV6 host to distinguish from the port, but for simplicity allow any
	// host to be encapsulated in which case the terminator MUST be the closing ']'
	ident->host = tmp;
	tmp = tmp2;

	if (!(*ident->host)) {
		ret = KS_STATUS_FAIL;
		goto done;
	}

	if (terminator == ':') {
		// port terminator
		ident->port = tmp;
		ident->portnum = atoi(ident->port);

		// next component must be the port, which may be terminated by an optional path, optional parameters, or the end of the uri
		// which means checking if a '/' for path, or ';' for parameters
		if (!(tmp2 = strpbrk(tmp, "/;"))) {
			// none of the terminators are found, treat the remaining string as a port
			goto done;
		}

		terminator = *tmp2;
		*tmp2++ = '\0';

		tmp = tmp2;

		if (!(*ident->port)) {
			ret = KS_STATUS_FAIL;
			goto done;
		}

		// @todo sscanf ident->port into ident->portnum and validate that it is a valid port number
	}

	if (terminator == '/') {
		// path terminator
		ident->path = tmp;

		// next component must be the path, which may be terminated by optional parameters, or the end of the uri
		// which means checking ';' for parameters
		if (!(tmp2 = strpbrk(tmp, ";"))) {
			// none of the terminators are found, treat the remaining string as a path
			goto done;
		}

		terminator = *tmp2;
		*tmp2++ = '\0';

		tmp = tmp2;

		if (!(*ident->path)) {
			ret = KS_STATUS_FAIL;
			goto done;
		}
	}

	if (terminator == ';') {
		// parameter terminator
		do {
			char *key = NULL;
			char *value = NULL;

			// next component must be the parameter key, which must be terminated by mandatory '=', end of the uri is an error
			// which means checking '=' for key terminator
			key = tmp;
			if (!(tmp = strpbrk(tmp, "="))) {
				ret = KS_STATUS_FAIL;
				goto done;
			}
			*tmp++ = '\0';

			// next component must be the parameter value, which may be terminated by another parameter terminator ';', or the end of the uri
			// if it is the end of the uri, then the parameter loop will be exited
			value = tmp;
			if ((tmp = strpbrk(tmp, ";"))) {
				*tmp++ = '\0';
			}

			// create th parameters hash if it does not already exist and add the parameter entry to it, note the key and value are both actually part
			// of the duplicated uri for components and will be cleaned up with the single string so the hash must not free the key or value itself
			if (!ident->parameters) ks_hash_create(&ident->parameters, KS_HASH_MODE_CASE_INSENSITIVE, KS_HASH_FLAG_NOLOCK | KS_HASH_FLAG_DUP_CHECK, pool);
			ks_hash_insert(ident->parameters, (void *)key, (void *)value);
		} while (tmp);
	}

done:
	if (ret != KS_STATUS_SUCCESS)
		swclt_ident_destroy(ident);
	return ret;
}

SWCLT_DECLARE(void) swclt_ident_to_str(
		swclt_ident_t *ident,
	   	ks_pool_t *pool,
	   	char **str,
	   	swclt_ident_to_str_flags_t flags)
{
	ks_sb_t *sb = NULL;

	ks_assert(ident);
	ks_assert(str);
	ks_assert(flags);

	ks_sb_create(&sb, pool, 64);

	if ((flags & SWCLT_IDENT_TOSTRING_SCHEME) && ident->scheme) {
		ks_sb_append(sb, ident->scheme);
		ks_sb_append(sb, "://");
	}
	if ((flags & SWCLT_IDENT_TOSTRING_USER) && ident->user) {
		ks_sb_append(sb, ident->user);
		ks_sb_append(sb, "@");
	}
	if ((flags & SWCLT_IDENT_TOSTRING_HOST) && ident->host) {
		ks_sb_append(sb, ident->host);
	}
	if ((flags & SWCLT_IDENT_TOSTRING_PORT) && ident->port) {
		ks_sb_append(sb, ":");
		ks_sb_append(sb, ident->port);
	}
	if ((flags & SWCLT_IDENT_TOSTRING_PATH) && ident->path) {
		ks_sb_append(sb, "/");
		ks_sb_append(sb, ident->path);
	}
	if ((flags & SWCLT_IDENT_TOSTRING_PARAMETERS) && ident->parameters && ks_hash_count(ident->parameters) > 0) {
		for (ks_hash_iterator_t *it = ks_hash_first(ident->parameters, KS_UNLOCKED); it; it = ks_hash_next(&it)) {
			const char *key = NULL;
			const char *value = NULL;

			ks_hash_this(it, (const void **)&key, 0, (void **)value);

			ks_sb_append(sb, key);
			ks_sb_append(sb, "=");
			ks_sb_append(sb, value);
			ks_sb_append(sb, ";");
		}
	}
	*str = ks_pstrdup(pool, ks_sb_cstr(sb));

	ks_sb_destroy(&sb);
}

SWCLT_DECLARE(void) swclt_ident_destroy(swclt_ident_t *ident)
{
	ks_hash_destroy(&ident->parameters);
}
