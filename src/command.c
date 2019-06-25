/*
 * Copyright (c) 2018-2019 SignalWire, Inc
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
#include "signalwire-client-c/internal/command.h"
#include "libks/ks_atomic.h"

static void __context_describe_locked(swclt_cmd_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	switch (ctx->type) {
		case SWCLT_CMD_TYPE_REQUEST:
		{
			const char *json_str = ks_json_pprint(ctx->base.pool, ctx->request);
			snprintf(buffer, buffer_len, "SWCLT CMD RQU: method: %s Id: %s TTL: %ums params: %s", ctx->method, ks_uuid_thr_str(&ctx->id), ctx->response_ttl_ms, json_str);
			ks_pool_free(&json_str);
			break;
		}

		case SWCLT_CMD_TYPE_RESULT:
		{
			const char *json_str = ks_json_pprint(ctx->base.pool, ctx->reply.result);
			snprintf(buffer, buffer_len, "SWCLT CMD RPL: method: %s Id: %s result: %s", ctx->method, ks_uuid_thr_str(&ctx->id), json_str);
			ks_pool_free(&json_str);
			break;
		}
		case SWCLT_CMD_TYPE_ERROR:
		{
			const char *json_str = ks_json_pprint(ctx->base.pool, ctx->reply.error);
			snprintf(buffer, buffer_len, "SWCLT CMD ERR: method: %s Id: %s error: %s", ctx->method, ks_uuid_thr_str(&ctx->id), json_str);
			ks_pool_free(&json_str);
			break;
		}

		case SWCLT_CMD_TYPE_FAILURE:
		{
			snprintf(buffer, buffer_len, "SWCLT CMD FAIL: %s", ctx->failure_summary);
			break;
		}

		default:
			ks_abort_fmt("Unexpected command type: %lu", ctx->type);
	}
}

static void __raise_callback(swclt_cmd_ctx_t *ctx)
{
	swclt_cmd_cb_t cb = ctx->cb;
	void *cb_data = ctx->cb_data;

	/* Its a good idea not to hold on to this lock while we call back
	 * into other code so do that now */
	swclt_cmd_ctx_unlock(ctx);
	if (cb)
		cb(ctx->base.handle, cb_data);
	swclt_cmd_ctx_lock(ctx);
}


static void __report_failure(const char *file, int line, const char *tag, swclt_cmd_ctx_t *ctx, ks_status_t failure_status, const char *failure_fmt, va_list *ap)
{
	/* Reset previous allocation if set */
	ks_pool_free(&ctx->failure_reason);
	ks_pool_free(&ctx->failure_summary);

	ctx->type = SWCLT_CMD_TYPE_FAILURE;
	ctx->failure_status = failure_status;
	if (ap) {
		ctx->failure_reason = ks_vpprintf(ctx->base.pool, failure_fmt, *ap);
	} else {
		ctx->failure_reason = ks_pstrdup(ctx->base.pool, failure_fmt);
	}

#if defined(KS_BUILD_DEBUG)
	ctx->failure_file = file;
	ctx->failure_line = line;
	ctx->failure_tag = tag;
#endif

#if defined(KS_BUILD_DEBUG)
	ctx->failure_summary = ks_psprintf(ctx->base.pool, "%s (%lu) [%s:%lu:%s]", ctx->failure_reason, ctx->failure_status, ctx->failure_file, ctx->failure_line, ctx->failure_tag);
#else
	ctx->failure_summary = ks_psprintf(ctx->base.pool, "%s (%lu)", ctx->failure_reason, ctx->failure_status);
#endif

	ks_log(KS_LOG_WARNING, "Command was failed: %s", ctx->failure_summary);

	/* Raise the completion callback */
	__raise_callback(ctx);
}

static void __context_describe(swclt_cmd_ctx_t *ctx, char *buffer, ks_size_t buffer_len)
{
	swclt_cmd_ctx_lock(ctx);
	__context_describe_locked(ctx, buffer, buffer_len);
	swclt_cmd_ctx_unlock(ctx);
}

static ks_status_t __context_init_frame(swclt_cmd_ctx_t *ctx, swclt_frame_t frame)
{
	const char *method, *jsonrpc;
	ks_json_t *original_json;
	ks_status_t status;

	/* Grab the json out of the frame, but don't copy it yet, we'll slice off just
	 * the params portion */
	if (status = swclt_frame_get_json(frame, &original_json)) {
		ks_log(KS_LOG_CRIT, "Received invalid frame: %s", ks_handle_describe(frame));
		return status;
	}

#if defined(SWCLT_DEBUG_JSON)
	KS_JSON_PPRINT(ctx->base.pool, "Parsing incoming frame:", original_json);
#endif

	/* Now load the id, method, and verify it has at least a params structure in it as
	 * well as the jsonrpc tag */
	if (!(method = ks_json_get_object_cstr_def(original_json, "method", NULL))) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command construction, no method field present: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}
	if (!(ctx->request = ks_json_get_object_item(original_json, "params"))) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command construction, no params field present: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}
	if (!(jsonrpc = ks_json_get_object_cstr_def(original_json, "jsonrpc", NULL))) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command construction, no jsonrpc field present: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}
	ctx->id = ks_json_get_object_uuid(original_json, "id");
	if (ks_uuid_is_null(&ctx->id)) {
		ks_log(KS_LOG_WARNING, "Invalid frame given to command construction, no id (or null id) field: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}
	if (!(ctx->id_str = ks_uuid_str(ctx->base.pool, &ctx->id)))
		return KS_STATUS_NO_MEM;

	/* Right finally its valid, copy some things */
	if (!(ctx->method = ks_pstrdup(ctx->base.pool, method)))
		return KS_STATUS_NO_MEM;

	/* We just need the request portion, dupe just that, leave the rest in ownership
	 * of the frame */
	if (!(ctx->request = ks_json_pduplicate(ctx->base.pool, ks_json_lookup(original_json, 1, "params"), KS_TRUE))) {
		ks_log(KS_LOG_CRIT, "Failed to allocate json request from frame: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	return KS_STATUS_SUCCESS;
}

static ks_status_t __context_init(swclt_cmd_ctx_t *ctx, swclt_cmd_cb_t cb, void *cb_data, const char * const method,
		ks_json_t **request, uint32_t response_ttl_ms, uint32_t flags, ks_uuid_t uuid, swclt_frame_t frame)
{
	/* Stash their callback if they're doing async (null would imply blocking)*/
	ctx->cb = cb;
	ctx->cb_data = cb_data;

	ctx->response_ttl_ms = response_ttl_ms;

	/* We always start out as a request */
	ctx->type = SWCLT_CMD_TYPE_REQUEST;

	/* Now if they didn't specify anything its invalid, unless they're wanting to construct
	 * from a frame */
	if (!method || !request || !*request) {
		if (frame) {
			return __context_init_frame(ctx, frame);
		}
		ks_log(KS_LOG_CRIT, "Context init failed invalid arguments: %s", ks_handle_describe(frame));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	/* Take ownership of their request and null it out to indicate that */
	ctx->request = *request;
	*request = NULL;

	/* Always dupe their string, their method string is their rsponsibility but, typically
	 * it will be a constant literal anyway */
	if (!(ctx->method = ks_pstrdup(ctx->base.pool, method))) {
		/* Note we don't have to clean up here, context_deinit will get called by the
		 * handle template macro */
		return KS_STATUS_NO_MEM;
	}

	/* Generate a new uuid if the one they passed in is null */
	if (ks_uuid_is_null(&uuid))
		ks_uuid(&ctx->id);
	else
		ctx->id = uuid;

	if (!(ctx->id_str = ks_uuid_str(ctx->base.pool, &ctx->id)))
		return KS_STATUS_NO_MEM;

	ctx->flags = flags;
	return KS_STATUS_SUCCESS;
}

static void __context_deinit(swclt_cmd_ctx_t *ctx)
{
	ks_pool_free(&ctx->method);
	ks_pool_free(&ctx->id_str);
	ks_json_delete((ks_json_t **)&ctx->request);
	ks_json_delete(&ctx->reply.error);
	ks_pool_free(&ctx->failure_summary);
	ks_pool_free(&ctx->failure_reason);
}

static ks_json_t * __wrap_jsonrpc(swclt_cmd_ctx_t *ctx, const char *version,
	const char *method, const char *id, ks_json_t *params, ks_json_t *result, ks_json_t *error)
{
	ks_json_t *jsonrpc_object = ks_json_pcreate_object(ctx->base.pool);

	ks_assertd(version);
	ks_assertd(id);

	ks_assertd(ks_json_padd_string_to_object(ctx->base.pool, jsonrpc_object, "jsonrpc", version));
	ks_assertd(ks_json_padd_string_to_object(ctx->base.pool, jsonrpc_object, "id", id));
	if (method) {
		ks_assertd(!result);
		ks_assertd(!error);
		ks_assertd(ks_json_padd_string_to_object(ctx->base.pool, jsonrpc_object, "method", method));
		ks_assertd(ks_json_add_item_to_object(jsonrpc_object, "params", params));
	} else if (result) {
		ks_assertd(!params);
		ks_assertd(!method);
		ks_assertd(!error);
		ks_assertd(ks_json_add_item_to_object(jsonrpc_object, "result", result));
	} else {
		ks_assertd(error);
		ks_assertd(!params);
		ks_assertd(!method);
		ks_assertd(!result);
		ks_assertd(ks_json_add_item_to_object(jsonrpc_object, "error", error));
	}

	return jsonrpc_object;
}

static ks_status_t __print_request(swclt_cmd_ctx_t *ctx, ks_pool_t *pool, char ** const string)
{
	ks_json_t *jsonrpc_request;

	if (!pool)
		pool = ctx->base.pool;

	jsonrpc_request = __wrap_jsonrpc(ctx, "2.0", ctx->method,
			ctx->id_str, ks_json_pduplicate(pool, (ks_json_t *)ctx->request, KS_TRUE), NULL, NULL);
	if (!jsonrpc_request)
		return KS_STATUS_NO_MEM;
	*string = ks_json_pprint_unformatted(pool, jsonrpc_request);
	if (!*string) {
		ks_json_delete(&jsonrpc_request);
		return KS_STATUS_NO_MEM;
	}
	ks_json_delete(&jsonrpc_request);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __print_result(swclt_cmd_ctx_t *ctx, ks_pool_t *pool, char **string)
{
	ks_json_t *jsonrpc_result;
	if (ctx->type != SWCLT_CMD_TYPE_RESULT) {
		ks_log(KS_LOG_WARNING, "Attempt to print incorrect result type, command type is: %s", swclt_cmd_type_str(ctx->type));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	if (!pool)
		pool = ctx->base.pool;

	jsonrpc_result = __wrap_jsonrpc(ctx, "2.0", NULL,
		ctx->id_str, NULL, ks_json_pduplicate(pool, ctx->reply.result, KS_TRUE), NULL);
	if (!jsonrpc_result)
		return KS_STATUS_NO_MEM;
	*string = ks_json_pprint_unformatted(ctx->base.pool, jsonrpc_result);
	if (!*string) {
		ks_json_delete(&jsonrpc_result);
		return KS_STATUS_NO_MEM;
	}
	ks_json_delete(&jsonrpc_result);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __print_error(swclt_cmd_ctx_t *ctx, ks_pool_t *pool, char **string)
{
	ks_json_t *jsonrpc_error;

	if (ctx->type != SWCLT_CMD_TYPE_ERROR) {
		ks_log(KS_LOG_WARNING, "Attempt to print incorrect error type, command type is: %s", swclt_cmd_type_str(ctx->type));
		return KS_STATUS_INVALID_ARGUMENT;
	}

	if (!pool)
		pool = ctx->base.pool;

	jsonrpc_error = __wrap_jsonrpc(ctx, "2.0", NULL,
		ctx->id_str, NULL, NULL, ks_json_pduplicate(pool, ctx->reply.error, KS_TRUE));
	if (!jsonrpc_error)
		return KS_STATUS_NO_MEM;
	*string = ks_json_pprint_unformatted(ctx->base.pool, jsonrpc_error);
	if (!*string) {
		ks_json_delete(&jsonrpc_error);
		return KS_STATUS_NO_MEM;
	}
	ks_json_delete(&jsonrpc_error);

	return KS_STATUS_SUCCESS;
}

static ks_status_t __set_result(swclt_cmd_ctx_t *ctx, ks_json_t **result)
{
	/* In case someone calls this twice free the previous one */
	ks_json_delete(&ctx->reply.result);

	/* Take ownership of their result json blob and assign it */
	ctx->reply.result = *result;
	*result = NULL;

	/* Our command type is now result */
	ctx->type = SWCLT_CMD_TYPE_RESULT;

	return KS_STATUS_SUCCESS;
}

static ks_status_t __set_error(swclt_cmd_ctx_t *ctx, ks_json_t **error)
{
	/* In case someone calls this twice free the previous one */
	ks_json_delete(&ctx->reply.error);

	/* Take ownership of their error json blob and assign it */
	ctx->reply.error = *error;
	*error = NULL;

	/* Our command type is now error */
	ctx->type = SWCLT_CMD_TYPE_ERROR;

	return KS_STATUS_SUCCESS;
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_create_frame(
	swclt_cmd_t *cmd,
   	swclt_cmd_cb_t cb,
   	void *cb_data,
	swclt_frame_t frame,
   	uint32_t response_ttl_ms,
   	uint32_t flags,
   	const char *file,
   	int line,
   	const char *tag)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M_TAG(
		NULL,
		file,
		line,
		tag,
		SWCLT_HTYPE_CMD,
		cmd,
		swclt_cmd_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		cb,
		cb_data,
		NULL,
		NULL,
		response_ttl_ms,
		flags,
		ks_uuid_null(),
		frame);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_create_ex(
	swclt_cmd_t *cmd,
	ks_pool_t **pool,
	swclt_cmd_cb_t cb,
	void *cb_data,
	const char * const method,
	ks_json_t **request,
	uint32_t response_ttl_ms,
	uint32_t flags,
	ks_uuid_t id,
	const char *file,
	int line,
	const char *tag)
{
	SWCLT_HANDLE_ALLOC_TEMPLATE_M_TAG(
		pool,
		file,
		line,
		tag,
		SWCLT_HTYPE_CMD,
		cmd,
		swclt_cmd_ctx_t,
		SWCLT_HSTATE_NORMAL,
		__context_describe,
		__context_deinit,
		__context_init,
		cb,
		cb_data,
		method,
		request,
		response_ttl_ms,
		flags,
		id,
		KS_NULL_HANDLE);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_create(
	swclt_cmd_t *cmd,
	const char * const method,
	ks_json_t **request,
	uint32_t response_ttl_ms,
	uint32_t flags,
	const char *file,
	int line,
	const char *tag)
{
	return __swclt_cmd_create_ex(cmd, NULL, NULL, NULL, method, request, response_ttl_ms, flags, ks_uuid_null(), file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_cb(swclt_cmd_t cmd, swclt_cmd_cb_t *cb, void **cb_data, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag)

	swclt_cmd_ctx_lock(ctx);
	*cb = ctx->cb;
	*cb_data = ctx->cb_data;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag)
}


SWCLT_DECLARE(ks_status_t) __swclt_cmd_print(swclt_cmd_t cmd, ks_pool_t *pool, char **string, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag)

	swclt_cmd_ctx_lock(ctx);

	switch (ctx->type) {
	case SWCLT_CMD_TYPE_REQUEST:
		status = __print_request(ctx, pool, string);
		break;
	case SWCLT_CMD_TYPE_RESULT:
		status = __print_result(ctx, pool, string);
		break;
	case SWCLT_CMD_TYPE_ERROR:
		status = __print_error(ctx, pool, string);
		break;
	default:
		//status = KS_STATUS_INVALID_ARGUMENT;
		ks_abort_fmt("Unexpected command context type: %lu", ctx->type);
		break;
	}

	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_submit_time(swclt_cmd_t cmd, ks_time_t submit_time, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	ctx->submit_time = submit_time;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_cb(swclt_cmd_t cmd, swclt_cmd_cb_t cb, void *cb_data, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	ctx->cb = cb;
	ctx->cb_data = cb_data;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_ttl(swclt_cmd_t cmd, uint32_t response_ttl_ms, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	ctx->response_ttl_ms = response_ttl_ms;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_ttl(swclt_cmd_t cmd, uint32_t *response_ttl_ms, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	*response_ttl_ms = ctx->response_ttl_ms;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_submit_time(swclt_cmd_t cmd, ks_time_t *submit_time, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	*submit_time = ctx->submit_time;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_id(swclt_cmd_t cmd, ks_uuid_t *id, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	*id = ctx->id;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_flags(swclt_cmd_t cmd, uint32_t *flags, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	*flags = ctx->flags;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_lookup_parse(const char *file, int line, const char *tag, swclt_cmd_t cmd, ks_pool_t *pool,
		swclt_cmd_parse_cb_t parse_cb, void **structure, int components, ...)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);
	va_list argptr;
	ks_json_t *obj;

	swclt_cmd_ctx_lock(ctx);

	/* If no pool specified, assume our own */
	if (!pool)
		pool = ctx->base.pool;

	va_start(argptr, components);

	if (ctx->type == SWCLT_CMD_TYPE_REQUEST) {
		if (!(obj = ks_json_valookup(ctx->request, components, argptr))) {
			status = KS_STATUS_NOT_FOUND;
			goto done;
		}
	} else if (ctx->type == SWCLT_CMD_TYPE_RESULT) {
		if (!(obj = ks_json_valookup(ctx->reply.result, components, argptr))) {
			status = KS_STATUS_NOT_FOUND;
			goto done;
		}
	} else {
		status = KS_STATUS_INVALID_ARGUMENT;
		ks_log(KS_LOG_CRIT, "Failed to lookup and parse: %s:%lu:%s", file, line, tag);
		goto done;
	}

	status = parse_cb(pool, obj, structure);

done:
	swclt_cmd_ctx_unlock(ctx);
	va_end(argptr);
	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_request(swclt_cmd_t cmd, const ks_json_t **request, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	*request = (ks_json_t *)ctx->request;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_error(swclt_cmd_t cmd, const ks_json_t **error, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);

	if (ctx->type != SWCLT_CMD_TYPE_ERROR) {
		status = KS_STATUS_INVALID_ARGUMENT;
		ks_log(KS_LOG_CRIT, "Invalid type except SWCLT_CMD_TYPE_ERROR");
		goto done;
	}

	*error = ctx->reply.error;

done:
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_result(swclt_cmd_t cmd, const ks_json_t **result, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);

	if (ctx->type != SWCLT_CMD_TYPE_RESULT) {
		status = KS_STATUS_INVALID_ARGUMENT;
		ks_log(KS_LOG_CRIT, "Invalid type expected result");
		goto done;
	}

	*result = ctx->reply.result;

done:
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_method(swclt_cmd_t cmd, const char **method, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	*method = ctx->method;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_type(swclt_cmd_t cmd, SWCLT_CMD_TYPE *type,
	const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	*type = ctx->type;
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_failure_info(swclt_cmd_t cmd, ks_status_t *failure_status,
	const char **failure_reason, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);
	swclt_cmd_ctx_lock(ctx);

	*failure_status = ctx->failure_status;
	*failure_reason = ctx->failure_reason;

	swclt_cmd_ctx_unlock(ctx);
	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_report_failure(swclt_cmd_t cmd, ks_status_t failure_status,
	const char *failure_description, const char *file, int line, const char* tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	__report_failure(file, line, tag, ctx, failure_status, failure_description, NULL);
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_report_failure_fmt(const char *file, int line, const char* tag, swclt_cmd_t cmd, ks_status_t failure_status, const char *failure_fmt, ...)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);
	va_list ap;
	va_start(ap, failure_fmt);

	swclt_cmd_ctx_lock(ctx);
	__report_failure(file, line, tag, ctx, failure_status, failure_fmt, &ap);
	swclt_cmd_ctx_unlock(ctx);

	va_end(ap);
	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_result(swclt_cmd_t cmd, ks_json_t **result, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	__set_result(ctx, result);
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_set_error(swclt_cmd_t cmd, ks_json_t **error, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);
	__set_error(ctx, error);
	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
}

SWCLT_DECLARE(ks_status_t) __swclt_cmd_parse_reply_frame(swclt_cmd_t cmd, swclt_frame_t frame, ks_bool_t *async, const char *file, int line, const char *tag)
{
	SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag);

	swclt_cmd_ctx_lock(ctx);

	/* Convert the frame to json */
	ks_json_t *reply, *result, *error;

	/* Get the json out of the frame */
	if (status = swclt_frame_to_json(frame, ctx->base.pool, &reply)) {
		ctx->failure_status = status;
		ctx->failure_reason = (char *)ks_pstrdup(ctx->base.pool, "Failed to parse the frame");
		goto done;
	}

	/* Now lets see if it was an error or not */
	if (result = ks_json_get_object_item(reply, "result")) {
		/* Hooray success */
		ctx->reply.result = result;
		ctx->type = SWCLT_CMD_TYPE_RESULT;
	} else if (error = ks_json_get_object_item(reply, "error")) {
		/* Boo error */
		ctx->reply.error = error;
		ctx->type = SWCLT_CMD_TYPE_ERROR;
	} else {
		/* uhh wut */
		status = ctx->failure_status = KS_STATUS_INVALID_ARGUMENT;
		ks_log(KS_LOG_CRIT, "Failed to parse reply cmd");
		ctx->failure_reason = ks_pstrdup(ctx->base.pool, "The result did not contain an error or result key");
	}

	/* Command has completed, call the callback */
	if (ctx->cb) {
		*async = KS_TRUE;
		__raise_callback(ctx);
	}

done:
	if (status) {
		/* Got an invalid frame, set ourselves to failure with
		 * the status built in */
		ctx->type = SWCLT_CMD_TYPE_FAILURE;
		__raise_callback(ctx);
	}

	swclt_cmd_ctx_unlock(ctx);

	SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag);
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
