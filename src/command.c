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

#include "signalwire-client-c/client.h"
#include "libks/ks_atomic.h"

#define ks_time_now_ms() ks_time_ms(ks_time_now())

typedef struct swclt_cmd_future {
	ks_pool_t *pool;
	ks_cond_t *cond;
	swclt_cmd_reply_t *reply;
	uint32_t response_ttl_ms;
	ks_bool_t got_reply;
	ks_uuid_t cmd_id;
	ks_bool_t destroy;
} swclt_cmd_future_t;

static void future_cmd_cb(swclt_cmd_reply_t *cmd_reply, void *cb_data)
{
	swclt_cmd_future_t *future = (swclt_cmd_future_t *)cb_data;
	ks_bool_t destroy;
	ks_cond_lock(future->cond);
	future->got_reply = KS_TRUE;
	future->reply = cmd_reply;
	destroy = future->destroy;
	if (destroy) {
		// waiter has given up waiting for reply, clean things up
		ks_cond_unlock(future->cond);
		swclt_cmd_future_destroy(&future);
	} else {
		// notify waiter that the reply has arrived
		ks_cond_broadcast(future->cond);
		ks_cond_unlock(future->cond);
	}
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_future_get(swclt_cmd_future_t *future, swclt_cmd_reply_t **reply)
{
	ks_status_t status;
	if (!future->response_ttl_ms) {
		return KS_STATUS_FAIL;
	}
	ks_time_t expiration_ms = ks_time_now_ms() + future->response_ttl_ms + 5000;
	ks_cond_lock(future->cond);
	while (!future->reply && expiration_ms > ks_time_now_ms()) {
		ks_cond_timedwait(future->cond, future->response_ttl_ms);
	}
	status = swclt_cmd_reply_ok(future->reply);
	if (reply && future->reply) {
		*reply = future->reply;
		future->reply = NULL;
	}
	ks_cond_unlock(future->cond);
	return status;
}

SWCLT_DECLARE(ks_uuid_t) swclt_cmd_future_get_id(swclt_cmd_future_t *future)
{
	return future->cmd_id;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_future_destroy(swclt_cmd_future_t **futureP)
{
	if (futureP && *futureP) {
		swclt_cmd_future_t *future = *futureP;
		ks_cond_lock(future->cond);
		if (future->got_reply) {
			// destroy now
			ks_cond_unlock(future->cond);
			ks_pool_t *pool = future->pool;
			ks_cond_destroy(&future->cond);
			swclt_cmd_reply_destroy(&future->reply);
			ks_pool_close(&pool);
		} else {
			// request destroy when reply arrives
			future->destroy = KS_TRUE;
			ks_cond_unlock(future->cond);
		}
		*futureP = NULL;
	}
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_future_create(swclt_cmd_future_t **futureP, swclt_cmd_t *cmd)
{
	ks_pool_t *pool = NULL;
	ks_pool_open(&pool);
	swclt_cmd_future_t *future = ks_pool_alloc(pool, sizeof(swclt_cmd_future_t));
	future->pool = pool;
	future->response_ttl_ms = cmd->response_ttl_ms;
	ks_cond_create(&future->cond, pool);
	future->reply = NULL;
	cmd->cb = future_cmd_cb;
	cmd->cb_data = future;
	*futureP = future;
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_create(swclt_cmd_reply_t **reply)
{
	if (reply) {
		ks_pool_t *pool = NULL;
		ks_pool_open(&pool);
		*reply = ks_pool_alloc(pool, sizeof(swclt_cmd_reply_t));
		(*reply)->pool = pool;
		return KS_STATUS_SUCCESS;
	}
	return KS_STATUS_ARG_INVALID;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_parse(swclt_cmd_reply_t *reply, ks_pool_t *pool,
		swclt_cmd_parse_cb_t parse_cb, void **structure)
{
	if (!pool) {
		pool = reply->pool;
	}
	return parse_cb(pool, reply->json, structure);
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_ok(swclt_cmd_reply_t *reply)
{
	if (reply && reply->type == SWCLT_CMD_TYPE_RESULT && reply->json) {
		return KS_STATUS_SUCCESS;
	}
	return KS_STATUS_GENERR;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_reply_destroy(swclt_cmd_reply_t **replyP)
{
	if (replyP && *replyP) {
		swclt_cmd_reply_t *reply = *replyP;
		ks_pool_t *pool = reply->pool;
		*replyP = NULL;
		ks_pool_free(&reply->failure_reason);
		ks_json_delete(&reply->json);
		ks_pool_close(&pool);
	}
}

SWCLT_DECLARE(char *) swclt_cmd_describe(swclt_cmd_t *cmd)
{
	char *str = NULL;
	switch (cmd->type) {
		case SWCLT_CMD_TYPE_REQUEST:
		{
			char *json_str = ks_json_print(cmd->json);
			str = ks_psprintf(cmd->pool, "SWCLT CMD RQU: method: %s Id: %s TTL: %ums params: %s", cmd->method, ks_uuid_thr_str(&cmd->id), cmd->response_ttl_ms, json_str);
			free(json_str);
			break;
		}

		case SWCLT_CMD_TYPE_RESULT:
		{
			char *json_str = ks_json_print(cmd->json);
			str = ks_psprintf(cmd->pool, "SWCLT CMD RPL: method: %s Id: %s result: %s", cmd->method, ks_uuid_thr_str(&cmd->id), json_str);
			free(json_str);
			break;
		}
		case SWCLT_CMD_TYPE_ERROR:
		{
			char *json_str = ks_json_print(cmd->json);
			str = ks_psprintf(cmd->pool, "SWCLT CMD ERR: method: %s Id: %s error: %s", cmd->method, ks_uuid_thr_str(&cmd->id), json_str);
			free(json_str);
			break;
		}

		case SWCLT_CMD_TYPE_FAILURE:
		{
			str = ks_psprintf(cmd->pool, "SWCLT CMD FAIL: method: %s Id: %s", cmd->method, ks_uuid_thr_str(&cmd->id));
			break;
		}

		default:
			str = ks_pstrdup(cmd->pool, "<Invalid SWCLT_CMD_TYPE>");
			break;
	}
	return str;
}

static void __raise_callback(swclt_cmd_t *cmd, swclt_cmd_reply_t **cmd_reply)
{
	if (cmd->cb) {
		cmd->cb(*cmd_reply, cmd->cb_data);
		*cmd_reply = NULL;
	} else {
		swclt_cmd_reply_destroy(cmd_reply);
	}
}

static void __report_failure(swclt_cmd_t *cmd, ks_status_t failure_status, const char *failure_fmt, va_list *ap)
{
	swclt_cmd_reply_t *reply = NULL;
	swclt_cmd_reply_create(&reply);
	reply->type = SWCLT_CMD_TYPE_FAILURE;
	reply->failure_status = failure_status;
	if (ap) {
		reply->failure_reason = ks_vpprintf(reply->pool, failure_fmt, *ap);
	} else {
		reply->failure_reason = ks_pstrdup(reply->pool, failure_fmt);
	}

	ks_log(KS_LOG_WARNING, "Command was failed: %s (%lu)", reply->failure_reason, reply->failure_status);

	/* Raise the completion callback */
	__raise_callback(cmd, &reply);
}

static ks_status_t __init_frame(swclt_cmd_t *cmd, swclt_frame_t *frame)
{
	const char *method, *jsonrpc;
	ks_json_t *original_json;
	ks_status_t status;
	ks_json_t *params;

	/* Grab the json out of the frame, but don't copy it yet, we'll slice off just
	 * the params portion */
	if (status = swclt_frame_to_json(frame, &original_json)) {
		ks_log(KS_LOG_CRIT, "Received invalid frame for command %s", ks_uuid_thr_str(&cmd->id));
		return status;
	}

#if defined(SWCLT_DEBUG_JSON)
	KS_JSON_PRINT(cmd->pool, "Parsing incoming frame:", original_json);
#endif

	/* Now load the id, method, and verify it has at least a params structure in it as
	 * well as the jsonrpc tag */
	if (!(method = ks_json_get_object_string(original_json, "method", NULL))) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command %s construction, no method field present", ks_uuid_thr_str(&cmd->id));
		status = KS_STATUS_INVALID_ARGUMENT;
		goto done;
	}
	if (!(params = ks_json_get_object_item(original_json, "params"))) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command %s construction, no params field present", ks_uuid_thr_str(&cmd->id));
		status = KS_STATUS_INVALID_ARGUMENT;
		goto done;
	}
	if (!(jsonrpc = ks_json_get_object_string(original_json, "jsonrpc", NULL))) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command %s construction, no jsonrpc field present", ks_uuid_thr_str(&cmd->id));
		status = KS_STATUS_INVALID_ARGUMENT;
		goto done;
	}
	cmd->id = ks_uuid_from_str(ks_json_get_object_string(original_json, "id", ""));
	if (ks_uuid_is_null(&cmd->id)) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command %s construction, no id (or null id) field", ks_uuid_thr_str(&cmd->id));
		status = KS_STATUS_INVALID_ARGUMENT;
		goto done;
	}
	if (!(cmd->id_str = ks_uuid_str(cmd->pool, &cmd->id))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* Right finally its valid, copy some things */
	if (!(cmd->method = ks_pstrdup(cmd->pool, method))) {
		status = KS_STATUS_NO_MEM;
		goto done;
	}

	/* We just need the request portion, dupe just that, leave the rest in ownership
	 * of the frame */
	if (!(cmd->json = ks_json_duplicate(params, KS_TRUE))) {
		ks_log(KS_LOG_CRIT, "Failed to allocate json request from command %s frame", ks_uuid_thr_str(&cmd->id));
		status = KS_STATUS_INVALID_ARGUMENT;
		goto done;
	}

done:
	if (original_json) {
		ks_json_delete(&original_json);
	}
	return status;
}

static ks_status_t __init_cmd(swclt_cmd_t **cmdP, swclt_cmd_cb_t cb, void *cb_data, const char * const method,
		ks_json_t **request, uint32_t response_ttl_ms, uint32_t flags, ks_uuid_t uuid, swclt_frame_t *frame)
{
	ks_pool_t *pool = NULL;
	ks_pool_open(&pool);
	swclt_cmd_t *cmd = ks_pool_alloc(pool, sizeof(swclt_cmd_t));
	*cmdP = cmd;
	cmd->pool = pool;

	/* Stash their callback if they're doing async (null would imply blocking)*/
	cmd->cb = cb;
	cmd->cb_data = cb_data;

	cmd->response_ttl_ms = response_ttl_ms;

	/* We always start out as a request */
	cmd->type = SWCLT_CMD_TYPE_REQUEST;

	/* Now if they didn't specify anything its invalid, unless they're wanting to construct
	 * from a frame */
	if (!method || !request || !*request) {
		if (frame) {
			ks_status_t status = __init_frame(cmd, frame);
			if (status != KS_STATUS_SUCCESS) {
				swclt_cmd_destroy(cmdP);
			}
			return status;
		}
		ks_log(KS_LOG_WARNING, "Command %s init failed invalid arguments", ks_uuid_thr_str(&uuid));
		swclt_cmd_destroy(cmdP);
		return KS_STATUS_INVALID_ARGUMENT;
	}

	/* Take ownership of their request and null it out to indicate that */
	cmd->json = *request;
	*request = NULL;

	cmd->method = ks_pstrdup(cmd->pool, method);

	/* Generate a new uuid if the one they passed in is null */
	if (ks_uuid_is_null(&uuid))
		ks_uuid(&cmd->id);
	else
		cmd->id = uuid;

	cmd->id_str = ks_uuid_str(cmd->pool, &cmd->id);

	cmd->flags = flags;

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(swclt_cmd_t *) swclt_cmd_duplicate(swclt_cmd_t *cmd)
{
	ks_pool_t *pool = NULL;
	ks_pool_open(&pool);
	swclt_cmd_t *dup_cmd = ks_pool_alloc(pool, sizeof(*dup_cmd));
	dup_cmd->pool = pool;
	if (cmd->json) {
		dup_cmd->json = ks_json_duplicate(cmd->json, KS_TRUE);
	}
	dup_cmd->type = cmd->type;
	dup_cmd->id = cmd->id;
	dup_cmd->id_str = ks_uuid_str(dup_cmd->pool, &dup_cmd->id);
	dup_cmd->response_ttl_ms = cmd->response_ttl_ms;
	if (cmd->method) {
		dup_cmd->method = ks_pstrdup(dup_cmd->pool, cmd->method);
	}
	dup_cmd->flags = cmd->flags;
	return dup_cmd;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_destroy(swclt_cmd_t **cmdP)
{
	if (cmdP && *cmdP) {
		swclt_cmd_t *cmd = *cmdP;
		*cmdP = NULL;
		ks_pool_t *pool = cmd->pool;
		ks_json_delete(&cmd->json);
		ks_pool_close(&pool);
	}
}

static ks_json_t * __wrap_jsonrpc(const char *version,
	const char *method, const char *id, ks_json_t *params, ks_json_t *result, ks_json_t *error)
{
	ks_json_t *jsonrpc_object = ks_json_create_object();

	ks_assertd(version);
	ks_assertd(id);

	ks_json_add_string_to_object(jsonrpc_object, "jsonrpc", version);
	ks_json_add_string_to_object(jsonrpc_object, "id", id);
	if (method) {
		ks_assertd(!result);
		ks_assertd(!error);
		ks_json_add_string_to_object(jsonrpc_object, "method", method);
		ks_json_add_item_to_object(jsonrpc_object, "params", params);
	} else if (result) {
		ks_assertd(!params);
		ks_assertd(!method);
		ks_assertd(!error);
		ks_json_add_item_to_object(jsonrpc_object, "result", result);
	} else {
		ks_assertd(error);
		ks_assertd(!params);
		ks_assertd(!method);
		ks_assertd(!result);
		ks_json_add_item_to_object(jsonrpc_object, "error", error);
	}

	return jsonrpc_object;
}

static ks_status_t __print_request(swclt_cmd_t *cmd, ks_pool_t *pool, char ** const string)
{
	ks_json_t *jsonrpc_request;

	if (!pool)
		pool = cmd->pool;

	jsonrpc_request = __wrap_jsonrpc("2.0", cmd->method,
			cmd->id_str, ks_json_duplicate(cmd->json, KS_TRUE), NULL, NULL);
	if (!jsonrpc_request)
		return KS_STATUS_NO_MEM;
	char *json_string = ks_json_print_unformatted(jsonrpc_request);
	if (!json_string) {
		ks_json_delete(&jsonrpc_request);
		return KS_STATUS_NO_MEM;
	}
	ks_json_delete(&jsonrpc_request);
	*string = ks_pstrdup(pool, json_string);
	free(json_string);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __print_result(swclt_cmd_t *cmd, ks_pool_t *pool, char **string)
{
	ks_json_t *jsonrpc_result;
	if (cmd->type != SWCLT_CMD_TYPE_RESULT) {
		ks_log(KS_LOG_WARNING, "Attempt to print incorrect result type, command type is: %s", swclt_cmd_type_str(cmd->type));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	if (!pool)
		pool = cmd->pool;

	jsonrpc_result = __wrap_jsonrpc("2.0", NULL,
		cmd->id_str, NULL, ks_json_duplicate(cmd->json, KS_TRUE), NULL);
	if (!jsonrpc_result)
		return KS_STATUS_NO_MEM;
	char *json_string = ks_json_print_unformatted(jsonrpc_result);
	if (!json_string) {
		ks_json_delete(&jsonrpc_result);
		return KS_STATUS_NO_MEM;
	}
	ks_json_delete(&jsonrpc_result);
	*string = ks_pstrdup(pool, json_string);
	free(json_string);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __print_error(swclt_cmd_t *cmd, ks_pool_t *pool, char **string)
{
	ks_json_t *jsonrpc_error;

	if (cmd->type != SWCLT_CMD_TYPE_ERROR) {
		ks_log(KS_LOG_WARNING, "Attempt to print incorrect error type, command type is: %s", swclt_cmd_type_str(cmd->type));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	if (!pool)
		pool = cmd->pool;

	jsonrpc_error = __wrap_jsonrpc("2.0", NULL,
		cmd->id_str, NULL, NULL, ks_json_duplicate(cmd->json, KS_TRUE));
	if (!jsonrpc_error)
		return KS_STATUS_NO_MEM;
	char *json_string = ks_json_print_unformatted(jsonrpc_error);
	if (!json_string) {
		ks_json_delete(&jsonrpc_error);
		return KS_STATUS_NO_MEM;
	}
	ks_json_delete(&jsonrpc_error);
	*string = ks_pstrdup(pool, json_string);
	free(json_string);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __set_result(swclt_cmd_t *cmd, ks_json_t **result)
{
	/* In case someone calls this twice free the previous one */
	ks_json_delete(&cmd->json);

	/* Take ownership of their result json blob and assign it */
	cmd->json = *result;
	*result = NULL;

	/* Our command type is now result */
	cmd->type = SWCLT_CMD_TYPE_RESULT;

	return KS_STATUS_SUCCESS;
}

static ks_status_t __set_error(swclt_cmd_t *cmd, ks_json_t **error)
{
	/* In case someone calls this twice free the previous one */
	ks_json_delete(&cmd->json);

	/* Take ownership of their error json blob and assign it */
	cmd->json = *error;
	*error = NULL;

	/* Our command type is now error */
	cmd->type = SWCLT_CMD_TYPE_ERROR;

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_create_frame(
	swclt_cmd_t **cmd,
	swclt_cmd_cb_t cb,
	void *cb_data,
	swclt_frame_t *frame,
	uint32_t response_ttl_ms,
	uint32_t flags)
{
	return __init_cmd(cmd, cb, cb_data, NULL, NULL, response_ttl_ms, flags, ks_uuid_null(), frame);
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_create_ex(
	swclt_cmd_t **cmd,
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char * const method,
	ks_json_t **request,
	uint32_t response_ttl_ms,
	uint32_t flags,
	ks_uuid_t id)
{
	__init_cmd(
		cmd,
		cb,
		cb_data,
		method,
		request,
		response_ttl_ms,
		flags,
		id,
		NULL);
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_create(
	swclt_cmd_t **cmd,
	const char * const method,
	ks_json_t **request,
	uint32_t response_ttl_ms,
	uint32_t flags)
{
	return swclt_cmd_create_ex(cmd, NULL, NULL, method, request, response_ttl_ms, flags, ks_uuid_null());
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_print(swclt_cmd_t *cmd, ks_pool_t *pool, char **string)
{
	ks_status_t status = KS_STATUS_SUCCESS;

	switch (cmd->type) {
	case SWCLT_CMD_TYPE_REQUEST:
		status = __print_request(cmd, pool, string);
		break;
	case SWCLT_CMD_TYPE_RESULT:
		status = __print_result(cmd, pool, string);
		break;
	case SWCLT_CMD_TYPE_ERROR:
		status = __print_error(cmd, pool, string);
		break;
	default:
		status = KS_STATUS_INVALID_ARGUMENT;
		*string = NULL;
		break;
	}

	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_set_cb(swclt_cmd_t *cmd, swclt_cmd_cb_t cb, void *cb_data)
{
	ks_status_t status = KS_STATUS_INVALID_ARGUMENT;
	if (!(cmd->flags & SWCLT_CMD_FLAG_NOREPLY)) {
		cmd->cb = cb;
		cmd->cb_data = cb_data;
		status = KS_STATUS_SUCCESS;
	}
	return status;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_set_ttl(swclt_cmd_t *cmd, uint32_t response_ttl_ms)
{
	cmd->response_ttl_ms = response_ttl_ms;
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_report_failure(swclt_cmd_t *cmd, ks_status_t failure_status,
	const char *failure_description)
{
	__report_failure(cmd, failure_status, failure_description, NULL);
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_report_failure_fmt(swclt_cmd_t *cmd, ks_status_t failure_status, const char *failure_fmt, ...)
{
	va_list ap;
	va_start(ap, failure_fmt);

	__report_failure(cmd, failure_status, failure_fmt, &ap);

	va_end(ap);
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_set_result(swclt_cmd_t *cmd, ks_json_t **result)
{
	__set_result(cmd, result);
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_set_error(swclt_cmd_t *cmd, ks_json_t **error)
{
	__set_error(cmd, error);
	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) swclt_cmd_parse_reply_frame(swclt_cmd_t *cmd, swclt_frame_t *frame)
{
	ks_status_t status = KS_STATUS_SUCCESS;
	ks_json_t *json = NULL;
	swclt_cmd_reply_t *reply = NULL;
	swclt_cmd_reply_create(&reply);

	if (cmd->type != SWCLT_CMD_TYPE_REQUEST) {
		ks_log(KS_LOG_INFO, "Discarding reply - command %s has already been finalized", ks_uuid_thr_str(&cmd->id));
		status = KS_STATUS_SUCCESS; // it is OK to continue
		goto done;
	}

	/* Get the json out of the frame */
	if (status = swclt_frame_to_json(frame, &json)) {
		reply->failure_status = status;
		reply->failure_reason = (char *)ks_pstrdup(reply->pool, "Failed to parse the frame");
		goto done;
	}

	/* Now lets see if it was an error or not */
	ks_json_t *result = NULL;
	if (result = ks_json_get_object_item(json, "result")) {
		/* Hooray success */
		reply->json = ks_json_duplicate(result, KS_TRUE);
		reply->type = SWCLT_CMD_TYPE_RESULT;
	} else if (result = ks_json_get_object_item(json, "error")) {
		/* Boo error */
		reply->json = ks_json_duplicate(result, KS_TRUE);
		reply->type = SWCLT_CMD_TYPE_ERROR;
	} else {
		/* uhh wut */
		status = reply->failure_status = KS_STATUS_INVALID_ARGUMENT;
		ks_log(KS_LOG_WARNING, "Failed to parse reply cmd");
		reply->failure_reason = ks_pstrdup(reply->pool, "The result did not contain an error or result key");
	}

done:
	if (status) {
		/* Got an invalid frame, set ourselves to failure with
		 * the status built in */
		reply->type = SWCLT_CMD_TYPE_FAILURE;
	}
	if (json) {
		ks_json_delete(&json);
	}
	__raise_callback(cmd, &reply);

	return KS_STATUS_SUCCESS;
}


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
