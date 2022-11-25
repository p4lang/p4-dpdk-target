/*
 * Copyright(c) 2022 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * @file fixed_function_int.h
 * @date
 *
 * Internal definitions for Fixed Functions.
 */
#ifndef __FIXED_FUNCTION_INT_H__
#define __FIXED_FUNCTION_INT_H__

#include <bf_types/bf_types.h>
#include <osdep/p4_sde_osdep_utils.h>
#include <osdep/p4_sde_osdep.h>

/*
 * Fixed Function Manager Types
 */
enum fixed_function_mgr {
	FF_MGR_PORT = 0,       /* Port Manager */
	FF_MGR_VPORT,          /* vPort Manager */
	FF_MGR_CRYPTO,         /* Crypto Manager */
	FF_MGR_INVALID
};

/*
 * Fixed Function Table Types
 *
 * According to the OpenConfig Standard, we are broadly
 * defining 2 Table Types : Configuration and State Tables
 *
 * Configuration Table : Configuration of Fixed Function
 * State Table : Retrieving State and Statistics of Fixed Function
 */
enum fixed_function_table_type {
	FIXED_FUNCTION_TABLE_TYPE_CONFIG = 0,  /* Config Table */
	FIXED_FUNCTION_TABLE_TYPE_STATE,       /* State Table */
	FIXED_FUNCTION_TABLE_TYPE_INVALID
};

/*
 * Generic Fixed Function Key Fields
 */
struct fixed_function_key_fields {
	uint32_t start_bit;
	uint32_t bit_width;
	char name[P4_SDE_NAME_LEN];
};

/*
 * Generic Fixed Function Data Fields
 */
struct fixed_function_data_fields {
	uint32_t start_bit;
	uint32_t bit_width;
	char name[P4_SDE_NAME_LEN];
};

/*
 * Fixed Function Table Context
 */
struct fixed_function_table_ctx {
	int size;
	int key_fields_count;
	int data_fields_count;
	enum fixed_function_table_type table_type;
	struct fixed_function_key_fields *key_fields;
	struct fixed_function_data_fields *data_fields;
	char name[P4_SDE_TABLE_NAME_LEN];
};

/*
 * Fixed Function Manager Context
 */
struct fixed_function_mgr_ctx {
	int num_tables;
	enum fixed_function_mgr ff_mgr;
	struct fixed_function_table_ctx *table_ctx;
};

/*
 * Fixed Function Context
 */
struct fixed_function_ctx {
	/* Maps the Fixed Function Type to a
	 * struct fixed_function_mgr_ctx
	 * fixed_function_mgr_ctx contains the context for the
	 * corresponding fixed function mgr
	 */
	p4_sde_map ff_mgr_ctx_map;
};

/*
 * Fixed Function Key Spec
 */
struct fixed_function_key_spec {
	uint16_t num_bytes; /* Number of bytes of data */
	uint8_t *array;     /* Bytes of data values */
};

/*
 * Fixed Function Data Spec
 */
struct fixed_function_data_spec {
	uint16_t num_bytes; /* Number of bytes of data */
	uint8_t *array;     /* Bytes of data values */
};

/**
 * Get Fixed Function Manager Ctx
 * @param ff_mgr Fixed Function Manager
 * @return fixed_function_mgr_ctx Fixed Function Manager Ctx
 */
struct fixed_function_mgr_ctx *get_fixed_function_mgr_ctx(
		enum fixed_function_mgr ff_mgr);

/**
 * Get Fixed Function Manager Table Ctx
 * @param ff_mgr_ctx Fixed Function Manager Ctx
 * @param tbl_type Table Type
 * @return fixed_function_table_ctx Table Ctx
 */
struct fixed_function_table_ctx *get_fixed_function_table_ctx
				(struct fixed_function_mgr_ctx *ff_mgr_ctx,
                                 enum fixed_function_table_type tbl_type);

/**
 * Encode Fixed Function Key Spec
 * @param key_spec Key Spec
 * @param start_bit Start Bit
 * @param bit_width Bit Width
 * @param key Key Field
 * @return status of the function call
 */
bf_status_t fixed_function_encode_key_spec(
                struct fixed_function_key_spec *key_spec,
                int start_bit,
                int bit_width,
                u8 *key);

/**
 * Encode Fixed Function Data Spec
 * @param data_spec Data Spec
 * @param start_bit Start Bit
 * @param bit_width Bit Width
 * @param data Data Field
 * @return status of the function call
 */
bf_status_t fixed_function_encode_data_spec(
		struct fixed_function_data_spec *data_spec,
		int start_bit,
		int bit_width,
		u8 *data);

/**
 * Decode Fixed Function Key Spec
 * @param key_spec Key Spec
 * @param start_bit Start Bit
 * @param bit_width Bit Width
 * @param key Decoded Key Field
 * @return status of the function call
 */
bf_status_t fixed_function_decode_key_spec(
		struct fixed_function_key_spec *key_spec,
		int start_bit,
		int bit_width,
		u8 *key);

/**
 * Decode Fixed Function Data Spec
 * @param data_spec Data Spec
 * @param start_bit Start Bit
 * @param bit_width Bit Width
 * @param data Decoded Data Field
 * @return status of the function call
 */
bf_status_t fixed_function_decode_data_spec(
		struct fixed_function_data_spec *data_spec,
		int start_bit,
		int bit_width,
		u8 *data);

/**
 * Get the Fixed Function Mgr Enum from
 * string
 * @param ff_name fixed function manager name string
 * @return fixed_function_mgr ff manager enum
 */
enum fixed_function_mgr get_fixed_function_mgr_enum(char *ff_name);
#endif /* __FIXED_FUNCTION_INT_H__ */
