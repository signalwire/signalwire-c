/*
 * Copyright (c) 2018-2022 SignalWire, Inc
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

/* A command is one of three types, request, result, error */
typedef enum {
	SWCLT_CMD_TYPE_INVALID,
	SWCLT_CMD_TYPE_ERROR,		/* Remote responded with an error */
	SWCLT_CMD_TYPE_REQUEST,		/* Initial state, constructed from a request, ready to submit */
	SWCLT_CMD_TYPE_RESULT,		/* Remote responded with a non error result */
	SWCLT_CMD_TYPE_FAILURE,		/* A failure ocurred that prevented remote from responding */
} SWCLT_CMD_TYPE;

typedef struct swclt_cmd swclt_cmd_t;

/* Flags control aspects of the command such as whether a reply is expected */
#define SWCLT_CMD_FLAG_NOREPLY KS_BIT_FLAG(0)
#define SWCLT_CMD_FLAG_MAX 1

struct swclt_cmd_reply {
	ks_pool_t *pool;
	ks_json_t *json;
	SWCLT_CMD_TYPE type;
	ks_status_t failure_status;
	char *failure_reason;
};

typedef struct swclt_cmd_reply swclt_cmd_reply_t;
typedef struct swclt_cmd_future swclt_cmd_future_t;

typedef ks_status_t (*swclt_cmd_parse_cb_t)(ks_pool_t *pool, ks_json_t * const payload, void **structure);
typedef void (*swclt_cmd_cb_t)(swclt_cmd_reply_t *cmd_reply, void *cb_data);

/* Ths client command context represents a commands state */
struct swclt_cmd {
	ks_pool_t *pool;
	/* When a command completes it calls the callback */
	swclt_cmd_cb_t cb;
	void *cb_data;

	/* The request ids, generated once on allocation */
	ks_uuid_t id;
	char *id_str;

	/* CMD flags e.g. whether we expect a reply or not */
	uint32_t flags : SWCLT_CMD_FLAG_MAX;

	/* Our method, established on construction */
	char *method;

	/* The request, a command always has one, as it is born from a request */
	ks_json_t *json;

	/* The command type can be request (when constructed) result (when remote replied
	 * with no error) and error (when remove replies with an error */
	SWCLT_CMD_TYPE type;

	/* This is the time to live value, when non zero, the command will fail if the
	 * response is not received within the appropriate window. */
	uint32_t response_ttl_ms;
};

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

KS_BEGIN_EXTERN_C

SWCLT_DECLARE(ks_status_t) swclt_cmd_set_cb(swclt_cmd_t *cmd, swclt_cmd_cb_t cb, void *cb_data);

SWCLT_DECLARE(ks_status_t) swclt_cmd_future_create(swclt_cmd_future_t **future, swclt_cmd_t *cmd);
SWCLT_DECLARE(ks_status_t) swclt_cmd_future_get(swclt_cmd_future_t *future, swclt_cmd_reply_t **reply);
SWCLT_DECLARE(ks_uuid_t) swclt_cmd_future_get_id(swclt_cmd_future_t *future);
SWCLT_DECLARE(ks_status_t) swclt_cmd_future_destroy(swclt_cmd_future_t **future);

SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_create(swclt_cmd_reply_t **reply);
SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_ok(swclt_cmd_reply_t *reply);
SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_parse(swclt_cmd_reply_t *reply, ks_pool_t *pool, swclt_cmd_parse_cb_t parse_cb, void **structure);
SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_destroy(swclt_cmd_reply_t **reply);

/* Create a command from a request, mandatory caller pool */
SWCLT_DECLARE(ks_status_t) swclt_cmd_create(swclt_cmd_t **cmd, const char * const method, ks_json_t **request,
	uint32_t response_ttl_ms, uint32_t flags);

/* Create a command from a request, and set a callback for async handling, mandatory caller pool */
SWCLT_DECLARE(ks_status_t) swclt_cmd_create_ex(swclt_cmd_t **cmd, swclt_cmd_cb_t cb, void *cb_data,
	const char * const method, ks_json_t **request, uint32_t response_ttl_ms, uint32_t flags, ks_uuid_t id);

/* Create a request from a frame w/optional callback */
SWCLT_DECLARE(ks_status_t) swclt_cmd_create_frame(
	swclt_cmd_t **cmd,
	swclt_cmd_cb_t cb,
	void *cb_data,
	swclt_frame_t *frame,
	uint32_t response_ttl_ms,
	uint32_t flags);

/* Duplicate a command, excluding its callback data */
SWCLT_DECLARE(swclt_cmd_t *) swclt_cmd_duplicate(swclt_cmd_t *cmd);
SWCLT_DECLARE(ks_status_t) swclt_cmd_destroy(swclt_cmd_t **cmd);
SWCLT_DECLARE(char *) swclt_cmd_describe(swclt_cmd_t *cmd);
SWCLT_DECLARE(ks_status_t) swclt_cmd_print(swclt_cmd_t *cmd, ks_pool_t *pool, char **string);
SWCLT_DECLARE(ks_status_t) swclt_cmd_set_result(swclt_cmd_t *cmd, ks_json_t **result);
SWCLT_DECLARE(ks_status_t) swclt_cmd_set_error(swclt_cmd_t *cmd, ks_json_t **result);
SWCLT_DECLARE(ks_status_t) swclt_cmd_set_ttl(swclt_cmd_t *cmd, uint32_t response_ttl_ms);
SWCLT_DECLARE(ks_status_t) swclt_cmd_parse_reply_frame(swclt_cmd_t *cmd, swclt_frame_t *frame);

SWCLT_DECLARE(ks_status_t) swclt_cmd_report_failure_fmt(swclt_cmd_t *cmd, ks_status_t failure_status, const char *failure_fmt, ...);
SWCLT_DECLARE(ks_status_t) swclt_cmd_report_failure(swclt_cmd_t *cmd, ks_status_t failure_status, const char *failure_description);

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
