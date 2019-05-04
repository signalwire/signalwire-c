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

/* Ths client command context represents a commands state */
struct swclt_cmd_ctx {
	swclt_handle_base_t base;

	/* When a command completes it calls the callback */
	swclt_cmd_cb_t cb;
	void *cb_data;

	/* Should we have a general failure processing a command, the
	 * type will go to SWCLT_CMD_TYPE_FAILURE and this status
	 * will be set along with a description buffer */
	ks_status_t failure_status;
	char *failure_reason;

	/* A summarized allocation of the failure so we can re-use it */
	char *failure_summary;

#if defined(KS_BUILD_DEBUG)
	/* Stashed on debug builds */
	const char *failure_file;
	int failure_line;
	const char *failure_tag;
#endif

	/* Each command is thread safe in terms of access to its apis
	 * using a spinlock to avoid excessive resource allocations. */
	ks_spinlock_t lock;

	/* The request ids, generated once on allocation */
	ks_uuid_t id;
	char *id_str;

	/* CMD flags e.g. whether we expect a reply or not */
	uint32_t flags : SWCLT_CMD_FLAG_MAX;

	/* Our method, established on construction */
	char *method;

	/* The request, a command always has one, as it is born from a request */
	ks_json_t *request;

	/* We unionize these since there can only be one kind of reply, an error
	 * or a result */
	union {
		ks_json_t *error;
		ks_json_t *result;
	} reply;

	/* The command type can be request (when constructed) result (when remote replied
	 * with no error) and error (when remove replies with an error */
	SWCLT_CMD_TYPE type;

	/* This is the time to live value, when non zero, the command will fail if the
	 * response is not received within the appropriate window. */
	uint32_t response_ttl_ms;

	/* This is a timestamp for when we got submitted in the connectins outstanding
	 * request hash. This is used by the service manager to timeout commands
	 * which have exceeded their time to live amount */
	ks_time_t submit_time;
};

/* Internal api for providing read only access to the command */
static inline ks_status_t __swclt_cmd_ctx_get(
	swclt_cmd_t cmd,
   	const swclt_cmd_ctx_t **ctx,
   	const char *file, int line, const char *tag)
{
	return __ks_handle_get(SWCLT_HTYPE_CMD, cmd, (ks_handle_base_t **)ctx, file, line, tag);
}

#define swclt_cmd_ctx_get(cmd, ctx)\
   	__swclt_cmd_ctx_get(cmd, ctx, __FILE__, __LINE__, __FILE__)

static inline void swclt_cmd_ctx_lock(const swclt_cmd_ctx_t *ctx) { ks_spinlock_acquire(&ctx->lock); }
static inline void swclt_cmd_ctx_unlock(const swclt_cmd_ctx_t *ctx) { ks_spinlock_release(&ctx->lock); }

static inline void __swclt_cmd_ctx_put(
   	const swclt_cmd_ctx_t **ctx,
   	const char *file, int line, const char *tag)
{
	__ks_handle_put(SWCLT_HTYPE_CMD, (ks_handle_base_t **)ctx, file, line, tag);
}

#define swclt_cmd_ctx_put(ctx)\
   	__swclt_cmd_ctx_put(ctx, __FILE__, __LINE__, __FILE__)

KS_END_EXTERN_C

/* Define helper macros to eliminate boiler code in handle wrapped apis */
#define SWCLT_CMD_SCOPE_BEG(cmd, ctx, status) \
	KS_HANDLE_SCOPE_BEG(SWCLT_HTYPE_CMD, cmd, swclt_cmd_ctx_t, ctx, status);

#define SWCLT_CMD_SCOPE_END(cmd, ctx, status) \
	KS_HANDLE_SCOPE_END(SWCLT_HTYPE_CMD, cmd, swclt_cmd_ctx_t, ctx, status);

#define SWCLT_CMD_SCOPE_BEG_TAG(cmd, ctx, status, file, line, tag) \
	KS_HANDLE_SCOPE_BEG_TAG(SWCLT_HTYPE_CMD, cmd, swclt_cmd_ctx_t, ctx, status, file, line, tag);

#define SWCLT_CMD_SCOPE_END_TAG(cmd, ctx, status, file, line, tag) \
	KS_HANDLE_SCOPE_END_TAG(SWCLT_HTYPE_CMD, cmd, swclt_cmd_ctx_t, ctx, status, file, line, tag);

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

