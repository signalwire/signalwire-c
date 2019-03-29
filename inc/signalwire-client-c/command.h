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

/* Obfuscate command internals */
typedef struct swclt_cmd_ctx swclt_cmd_ctx_t;

typedef ks_handle_t swclt_cmd_t;

/* A command is one of three types, request, result, error */
typedef enum {
	SWCLT_CMD_TYPE_INVALID,
	SWCLT_CMD_TYPE_ERROR,		/* Remote responded with an error */
	SWCLT_CMD_TYPE_REQUEST,		/* Initial state, constructed from a request, ready to submit */
	SWCLT_CMD_TYPE_RESULT,		/* Remote responded with a non error result */
	SWCLT_CMD_TYPE_FAILURE,		/* A failure ocurred that prevented remote from responding */
} SWCLT_CMD_TYPE;

static inline const char * swclt_cmd_type_str(SWCLT_CMD_TYPE type)
{
	switch (type) {
		case SWCLT_CMD_TYPE_INVALID:
			return "Invalid";
		case SWCLT_CMD_TYPE_ERROR:
			return "Error";
		case SWCLT_CMD_TYPE_REQUEST:
			return "Request";
		case SWCLT_CMD_TYPE_RESULT:
			return "Result";
		case SWCLT_CMD_TYPE_FAILURE:
			return "Failure";
		default:
			ks_debug_break();
			return "Unknown";
	}
}

/* Flags control aspects of the command such as whether a reply is expected */
#define SWCLT_CMD_FLAG_NOREPLY KS_BIT_FLAG(0)
#define SWCLT_CMD_FLAG_MAX 1

typedef ks_status_t (*swclt_cmd_parse_cb_t)(ks_pool_t *pool, const ks_json_t * const payload, void **structure);
typedef void (*swclt_cmd_cb_t)(swclt_cmd_t cmd, void *cb_data);

KS_BEGIN_EXTERN_C

SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_cb(swclt_cmd_t cmd, swclt_cmd_cb_t cb, void *cb_data,
	const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_cb(swclt_cmd_t cmd, swclt_cmd_cb_t *cb, void **cb_data,
	const char *file, int line, const char *tag);

/* Create a command from a request, optional caller pool */
SWCLT_DECLARE(ks_status_t) __swclt_cmd_create(swclt_cmd_t *cmd, const char * const method, ks_json_t **request,
	uint32_t response_ttl_ms, uint32_t flags, const char *file, int line, const char *tag);

#define swclt_cmd_create(cmd, method, request, response_ttl_ms, flags)	\
	__swclt_cmd_create(cmd, method, request, response_ttl_ms, flags, __FILE__, __LINE__, __PRETTY_FUNCTION__)

/* Create a command from a request, and set a callback for async handling, optional caller pool */
SWCLT_DECLARE(ks_status_t) __swclt_cmd_create_ex(swclt_cmd_t *cmd, ks_pool_t **pool,  swclt_cmd_cb_t cb, void *cb_data,
	const char * const method, ks_json_t **request, uint32_t response_ttl_ms, uint32_t flags, ks_uuid_t id,
	const char *file, int line, const char *tag);

#define swclt_cmd_create_ex(cmd, pool, cb, cb_data, method, request,response_ttl_ms, flags, id)	\
	__swclt_cmd_create_ex(cmd, pool, cb, cb_data, method, request, response_ttl_ms, flags, id, __FILE__, __LINE__, __PRETTY_FUNCTION__)

/* Create a request from af rame w/optional callback */
SWCLT_DECLARE(ks_status_t) __swclt_cmd_create_frame(
	swclt_cmd_t *cmd,
	swclt_cmd_cb_t cb,
	void *cb_data,
	swclt_frame_t frame,
	uint32_t response_ttl_ms,
	uint32_t flags,
	const char *file,
	int line,
	const char *tag);

#define swclt_cmd_create_frame(cmd, cb, cb_data, frame, frame_response_ttl_ms, flags)	\
	__swclt_cmd_create_frame(cmd, cb, cb_data, frame, frame_response_ttl_ms, flags, __FILE__, __LINE__, __PRETTY_FUNCTION__)

SWCLT_DECLARE(ks_status_t) __swclt_cmd_print(swclt_cmd_t cmd, ks_pool_t *pool, char **string, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_type(swclt_cmd_t cmd, SWCLT_CMD_TYPE *type, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_result(swclt_cmd_t cmd, ks_json_t **result, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_error(swclt_cmd_t cmd, ks_json_t **result, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_lookup_parse(const char *file, int line, const char *tag, swclt_cmd_t cmd, ks_pool_t *pool, swclt_cmd_parse_cb_t parse_cb, void **structure, int components, ...);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_error(swclt_cmd_t cmd, const ks_json_t **error, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_result(swclt_cmd_t cmd, const ks_json_t **result, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_request(swclt_cmd_t cmd, const ks_json_t **request, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_id(swclt_cmd_t cmd, ks_uuid_t *id, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_ttl(swclt_cmd_t cmd, uint32_t *response_ttl_ms, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_ttl(swclt_cmd_t cmd, uint32_t response_ttl_ms, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_submit_time(swclt_cmd_t cmd, ks_time_t *submit_time, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_submit_time(swclt_cmd_t cmd, ks_time_t submit_time, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_failure_info(swclt_cmd_t cmd, ks_status_t *failure_status, const char **failure_reason, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_flags(swclt_cmd_t cmd, uint32_t *flags, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_method(swclt_cmd_t cmd, const char **method, const char *file, int line, const char *tag);
SWCLT_DECLARE(ks_status_t) __swclt_cmd_parse_reply_frame(swclt_cmd_t cmd, swclt_frame_t frame, ks_bool_t *async, const char *file, int line, const char *tag);

#define swclt_cmd_set_cb(cmd, cb, cb_data) __swclt_cmd_set_cb(cmd, cb, cb_data, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_cb(cmd, cb, cb_data) __swclt_cmd_cb(cmd, cb, cb_data, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_print(cmd, pool, string) __swclt_cmd_print(cmd, pool, string, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_type(cmd, type) __swclt_cmd_type(cmd, type, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_result(cmd, result) __swclt_cmd_result(cmd, result, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_set_result(cmd, result) __swclt_cmd_set_result(cmd, result, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_set_error(cmd, error) __swclt_cmd_set_error(cmd, error, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_lookup_parse_m(cmd, pool, parse_cb, structure, components, ...)	__swclt_cmd_lookup_parse(__FILE__, __LINE__, __PRETTY_FUNCTION__, cmd, pool, parse_cb, structure, components, __VA_ARGS__)
#define swclt_cmd_lookup_parse_s(cmd, pool, parse_cb, structure)	__swclt_cmd_lookup_parse(__FILE__, __LINE__, __PRETTY_FUNCTION__, cmd, pool, parse_cb, structure, 0)
#define swclt_cmd_error(cmd, error) __swclt_cmd_error(cmd, error, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_request(cmd, request) __swclt_cmd_request(cmd, request, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_id(cmd, id) __swclt_cmd_id(cmd, id, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_ttl(cmd, response_ttl_ms) __swclt_cmd_ttl(cmd, response_ttl_ms, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_set_ttl(cmd, response_ttl_ms) __swclt_cmd_set_ttl(cmd, response_ttl_ms, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_submit_time(cmd, submit_time) __swclt_cmd_submit_time(cmd, submit_time, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_set_submit_time(cmd, submit_time) __swclt_cmd_set_submit_time(cmd, submit_time, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_failure_info(cmd, failure_status, failure_reason) __swclt_cmd_failure_info(cmd, failure_status, failure_reason, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_flags(cmd, flags) __swclt_cmd_flags(cmd, flags, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_method(cmd, method) __swclt_cmd_method(cmd, method, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_parse_reply_frame(cmd, frame, async) __swclt_cmd_parse_reply_frame(cmd, frame, async, __FILE__, __LINE__, __PRETTY_FUNCTION__)

SWCLT_DECLARE(ks_status_t) __swclt_cmd_report_failure_fmt(const char *file, int line, const char *tag, swclt_cmd_t cmd, ks_status_t failure_status, const char *failure_fmt, ...);
#define swclt_cmd_report_failure_fmt_s(cmd, status, error_fmt) __swclt_cmd_report_failure_fmt(__FILE__, __LINE__, __PRETTY_FUNCTION__, cmd, status, error_fmt)
#define swclt_cmd_report_failure_fmt_m(cmd, status, error_fmt, ...) __swclt_cmd_report_failure_fmt(__FILE__, __LINE__, __PRETTY_FUNCTION__, cmd, status, error_fmt, __VA_ARGS__)

SWCLT_DECLARE(ks_status_t) __swclt_cmd_report_failure(swclt_cmd_t cmd, ks_status_t failure_status, const char *failure_description, const char *file, int line, const char *tag);
#define swclt_cmd_report_failure(cmd, status, description) __swclt_cmd_report_failure(cmd, status, description, __FILE__, __LINE__, __PRETTY_FUNCTION__)

#define swclt_cmd_get(cmd, contextP)	__ks_handle_get(SWCLT_HTYPE_CMD, cmd, (ks_handle_base_t **)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)
#define swclt_cmd_put(contextP)			__ks_handle_put(SWCLT_HTYPE_CMD, (ks_handle_base_t **)contextP, __FILE__, __LINE__, __PRETTY_FUNCTION__)

KS_END_EXTERN_C

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
