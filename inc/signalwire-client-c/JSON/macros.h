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

/* CREATE - Constructs the type with default values. Makes a method
 * called function_name_##CREATE. Useful for generating the source
 * sturctures to later be fed into MARSHAL. */
#define SWCLT_JSON_ALLOC_BEG(function_name, target_type)								\
	static inline ks_status_t function_name##_ALLOC(ks_pool_t *pool,					\
		target_type **result)															\
	{																					\
                ks_status_t status = KS_STATUS_SUCCESS;\
		void (*release_cb)(target_type **) = function_name##_DESTROY;					\
		target_type *target = (target_type *)ks_pool_alloc(pool, sizeof(target_type));	\
																						\
		if (!target) {status = KS_STATUS_NO_MEM; goto end;}

#define SWCLT_JSON_ALLOC_CUSTOM(function_name, key)						\
	{																	\
		if ((status = function_name##_ALLOC(pool, &target->key))) { \
		  goto end;\
		}																\
	}

#define SWCLT_JSON_ALLOC_DEFAULT(key, value)							\
		target->key = value;


#define SWCLT_JSON_ALLOC_END()											\
  end:\
        if (status) {								\
	  if (target) release_cb(&target);				\
	  return status;\
        }\
	*result = target;													\
	return KS_STATUS_SUCCESS;											\
	}

/* PARSE - Populate a structure with distinct copies of all attributes from json.
 * Meant to be used in methods returning ks_status_t's.
 * Creates a function name function_name_ALLOC, expects a function_name_DESTROY function
 * to also exist. */
#define SWCLT_JSON_PARSE_BEG(function_name, target_type)										\
	static inline ks_status_t function_name##_PARSE(ks_pool_t *pool,							\
		const ks_json_t * const object, target_type **result)									\
	{																							\
		void (*release_cb)(target_type **) = (void(*)(target_type **))function_name##_DESTROY;	\
		target_type *target = (target_type *)ks_pool_alloc(pool, sizeof(target_type));			\
																								\
		if (!target) return KS_STATUS_NO_MEM;

#define SWCLT_JSON_PARSE_CUSTOM_OPT(function_name, key)							\
	{																			\
		ks_json_t *item = ks_json_get_object_item(object, #key);				\
		ks_status_t status;														\
																				\
		if (item) {																\
			if (!ks_json_type_is_object(item))  {								\
				release_cb(&target);											\
				return KS_STATUS_INVALID_ARGUMENT;								\
			}																	\
			if ((status = function_name##_PARSE(pool, object, &target->key))) { \
				release_cb(&target);											\
				return status;													\
			}																	\
		}																		\
	}

#define SWCLT_JSON_PARSE_UUID_OPT(key)										\
	{																		\
		ks_json_t *item = ks_json_get_object_item(object, #key);			\
		if (item) {															\
			if (!ks_json_type_is_string(item))  {							\
				release_cb(&target);										\
				return KS_STATUS_INVALID_ARGUMENT;							\
			}																\
			target->key = ks_json_value_uuid(item);						\
		}																	\
	}

#define SWCLT_JSON_PARSE_BOOL_OPT(key)									\
	{																	\
		ks_json_t *item = ks_json_get_object_item(object, #key);		\
		if (item) {														\
			if (!ks_json_type_is_bool(item))  {							\
				release_cb(&target);									\
				return KS_STATUS_INVALID_ARGUMENT;						\
			}															\
			target->key = ks_json_value_bool(item);					\
		}																\
	}

#define SWCLT_JSON_PARSE_INT_OPT(key)									\
	{																	\
		ks_json_t *item = ks_json_get_object_item(object, #key);		\
		if (item) {														\
				if (!ks_json_type_is_number(item))  {					\
					release_cb(&target);								\
					return KS_STATUS_INVALID_ARGUMENT;					\
			}															\
			*((int *)&target->key) = ks_json_value_number_int(item);	\
		}																\
	}

#define SWCLT_JSON_PARSE_INT_OPT_DEF(key, def)							\
	{																	\
		ks_json_t *item = ks_json_get_object_item(object, #key);		\
		if (item) {														\
				if (!ks_json_type_is_number(item))  {					\
					release_cb(&target);								\
					return KS_STATUS_INVALID_ARGUMENT;					\
			}															\
			*((int *)&target->key) = ks_json_value_number_int(item);	\
		} else *((int *)&target->key) = def;							\
	}

#define SWCLT_JSON_PARSE_STRING_OPT(key)								\
	{																	\
		const char *str = ks_json_get_object_cstr(object, #key);		\
		if (str)  {														\
			if (!(target->key = ks_pstrdup(pool, str))) {				\
				release_cb(&target);									\
				return KS_STATUS_NO_MEM;								\
			}															\
		}																\
	}

#define SWCLT_JSON_PARSE_ITEM_OPT(key)											\
	{																			\
		ks_json_t *item = ks_json_get_object_item(object, #key);				\
		if (item) {																\
			if (!(target->key = ks_json_pduplicate(pool, item, KS_TRUE)))	 {	\
				release_cb(&target);											\
				return KS_STATUS_NO_MEM;										\
			}																	\
		}																		\
	}

#define SWCLT_JSON_PARSE_CUSTOM(function_name, key)							\
	{																		\
		ks_json_t *item = ks_json_get_object_item(object, #key);			\
		ks_status_t status;													\
																			\
		if (!item || !ks_json_type_is_object(item))  {						\
			release_cb(&target);											\
			return KS_STATUS_INVALID_ARGUMENT;								\
		}																	\
		if ((status = function_name##_PARSE(pool, object, &target->key))) { \
			release_cb(&target);											\
			return status;													\
		}																	\
	}

#define SWCLT_JSON_PARSE_UUID(key)										\
	{																	\
		ks_json_t *item = ks_json_get_object_item(object, #key);		\
		if (!item || !ks_json_type_is_string(item))  {					\
			release_cb(&target);										\
			return KS_STATUS_INVALID_ARGUMENT;							\
		}																\
		target->key = ks_json_value_uuid(item);						\
	}

#define SWCLT_JSON_PARSE_BOOL(key)										\
	{																	\
		ks_json_t *item = ks_json_get_object_item(object, #key);		\
		if (!item || !ks_json_type_is_bool(item))  {					\
			release_cb(&target);										\
			return KS_STATUS_INVALID_ARGUMENT;							\
		}																\
		target->key = ks_json_value_bool(item);						\
	}

#define SWCLT_JSON_PARSE_DOUBLE(key)										\
	{																		\
		ks_json_t *item = ks_json_get_object_item(object, #key);			\
		if (!item || !ks_json_type_is_number(item))  {						\
			release_cb(&target);											\
			return KS_STATUS_INVALID_ARGUMENT;								\
		}																	\
		*((double *)&target->key) = ks_json_value_number_double(item);	\
	}

#define SWCLT_JSON_PARSE_INT_EX(source_key, target_key)						\
	{																		\
		ks_json_t *item = ks_json_get_object_item(object, #target_key);		\
		if (!item || !ks_json_type_is_number(item))  {						\
			release_cb(&target);											\
			return KS_STATUS_INVALID_ARGUMENT;								\
		}																	\
		*((int *)&target->source_key) = ks_json_value_number_int(item);	\
	}


#define SWCLT_JSON_PARSE_INT(key)										\
	{																	\
		ks_json_t *item = ks_json_get_object_item(object, #key);		\
		if (!item || !ks_json_type_is_number(item))  {					\
			release_cb(&target);										\
			return KS_STATUS_INVALID_ARGUMENT;							\
		}																\
		*((int *)&target->key) = ks_json_value_number_int(item);		\
	}

#define SWCLT_JSON_PARSE_STRING(key)									\
	{																	\
		const char *str = ks_json_get_object_cstr(object, #key);		\
		if (!str)  {													\
			release_cb(&target);										\
			return KS_STATUS_INVALID_ARGUMENT;							\
		}																\
		if (!(target->key = ks_pstrdup(pool, str))) {					\
			release_cb(&target);										\
			return KS_STATUS_NO_MEM;									\
		}																\
	}

#define SWCLT_JSON_PARSE_ITEM(key)											\
	{																		\
		ks_json_t *item = ks_json_get_object_item(object, #key);			\
		if (!item) {														\
			release_cb(&target);											\
			return KS_STATUS_INVALID_ARGUMENT;								\
		}																	\
		if (!(target->key = ks_json_pduplicate(pool, item, KS_TRUE)))	 {	\
			release_cb(&target);											\
			return KS_STATUS_NO_MEM;										\
		}																	\
	}

#define SWCLT_JSON_PARSE_END()											\
	*result = (void *)target;											\
	return KS_STATUS_SUCCESS;											\
	}

/* DESTROY - Opposite of alloc, cleanly tears down and sets items to null.
 * Creates a method called function_name_DESTROY. */
#define SWCLT_JSON_DESTROY_BEG(function_name, type)						\
	static inline void function_name##_DESTROY(type **__free_target)	\
	{																	\
		type *target;													\
		if (!__free_target || !*__free_target)							\
			return;														\
		target = *__free_target;

#define SWCLT_JSON_DESTROY_STRING(key)	ks_pool_free(&target->key);

#define SWCLT_JSON_DESTROY_ITEM(key)	ks_json_delete((ks_json_t **)&target->key);

#define SWCLT_JSON_DESTROY_UUID(key)	/* uuids are by value, null op */

#define SWCLT_JSON_DESTROY_BOOL(key)	/* bools are by value, null op */

#define SWCLT_JSON_DESTROY_INT(key)		/* ints are by value, null op */
#define SWCLT_JSON_DESTROY_INT_EX(source_key, target_key)		/* ints are by value, null op */

#define SWCLT_JSON_DESTROY_DOUBLE(key)	/* doubles are by value, null op */

#define SWCLT_JSON_DESTROY_CUSTOM(function_name, key)	\
	function_name##_DESTROY(&target->key);

#define SWCLT_JSON_DESTROY_END()			\
	ks_pool_free(__free_target);			\
}

/* MARHSAL - Converts the type to a json object, opposite of alloc/parse.
 * Creates an inline function called function_name_MARSHAL. */
#define SWCLT_JSON_MARSHAL_BEG(function_name, target_type)											\
	static inline ks_json_t * function_name##_MARSHAL(ks_pool_t *pool, target_type *source)			\
	{																								\
		if (!source)																				\
			return NULL;																			\
		ks_json_t *target = ks_json_pcreate_object(pool);											\
		if (!target)																				\
			return NULL;

#define SWCLT_JSON_MARSHAL_UUID(key)										\
	if (!(ks_json_padd_uuid_to_object(pool, target, #key, source->key))) {	\
		ks_json_delete(&target);											\
		return NULL;														\
	}

#define SWCLT_JSON_MARSHAL_BOOL(key)											\
	if (!(ks_json_padd_bool_to_object(pool, target, #key, source->key))) {		\
		ks_json_delete(&target);												\
		return NULL;															\
	}

#define SWCLT_JSON_MARSHAL_DOUBLE(key)													\
	if (!(ks_json_padd_number_to_object(pool, target, #key, (double)source->key))) {		\
		ks_json_delete(&target);														\
		return NULL;																	\
	}

#define SWCLT_JSON_MARSHAL_INT_EX(source_key, target_key)										\
	if (!(ks_json_padd_number_to_object(pool, target, #target_key, (int)source->source_key))) {	\
		ks_json_delete(&target);																\
		return NULL;																			\
	}

#define SWCLT_JSON_MARSHAL_INT(key)														\
	if (!(ks_json_padd_number_to_object(pool, target, #key, (int)source->key))) {		\
		ks_json_delete(&target);														\
		return NULL;																	\
	}

#define SWCLT_JSON_MARSHAL_CUSTOM(function_name, key)							\
	{																			\
		ks_json_t *__custom_obj;												\
		if (!(__custom_obj = function_name##_MARSHAL(pool, source->key))) {		\
			ks_json_delete(&target);											\
			return NULL;														\
		}																		\
		if (!ks_json_add_item_to_object(target, #key, __custom_obj)) {			\
			ks_json_delete(&target);											\
			return NULL;														\
		}																		\
	}

#define SWCLT_JSON_MARSHAL_STRING_OPT(key)								\
	if (source->key) {													\
		if (!(ks_json_padd_string_to_object(pool, target, #key, source->key))) {	\
			ks_json_delete(&target);									\
			return NULL;												\
		}																\
		source->key = NULL;												\
	}

#define SWCLT_JSON_MARSHAL_STRING(key)											\
	if (!(ks_json_padd_string_to_object(pool, target, #key, source->key))) {		\
		ks_json_delete(&target);												\
		return NULL;															\
	}

#define SWCLT_JSON_MARSHAL_ITEM_OPT(key)								\
	if (source->key) {													\
		if (!(ks_json_add_item_to_object(target, #key, source->key))) {	\
			ks_json_delete(&target);									\
			return NULL;												\
		}																\
		source->key = NULL;												\
	}

#define SWCLT_JSON_MARSHAL_ITEM(key)								\
	if (!(ks_json_add_item_to_object(target, #key, source->key))){	\
		ks_json_delete(&target);									\
		return NULL;												\
	}																\
	source->key = NULL;

#define SWCLT_JSON_MARSHAL_END()									\
	return target;													\
	}

KS_END_EXTERN_C
