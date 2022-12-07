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
/* tdi_includes */
#include <tdi/common/tdi_info.hpp>
#include "tdi_fixed_mgr_intf.hpp"
#include <tdi/common/tdi_defs.h>
#include <port_mgr/port_mgr_port.h>

namespace tdi {
namespace pna {
namespace rt {

std::unique_ptr<IFixedFunctionMgrIntf> IFixedFunctionMgrIntf::instance = nullptr;
std::mutex IFixedFunctionMgrIntf::fixed_mgr_intf_mtx;

// APIs for fixed function
pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntAdd(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		const char *table_name,
		struct fixed_function_key_spec  *key_spec,
		struct fixed_function_data_spec *data_spec)
{
        return fixed_func_mgr_ent_add(sess_hdl,
                                      dev_tgt,
			              table_name,
			              key_spec,
			              data_spec);
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntDel(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		const char *table_name,
		struct fixed_function_key_spec *key_spec)
{
        return fixed_func_mgr_ent_del(sess_hdl,
                                      dev_tgt,
                                      table_name,
                                      key_spec);
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntGetDefaultEntry(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		const char *table_name,
		struct fixed_function_data_spec *data_spec)
{
	return fixed_func_mgr_get_default_entry(sess_hdl,
			                        dev_tgt,
			                        table_name,
			                        data_spec);
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntGet(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		pipe_val_lookup_tbl_hdl_t tbl_hdl,
		pipe_val_lookup_ent_hdl_t ent_hdl,
		pipe_tbl_match_spec_t *match_spec,
		pipe_data_spec_t *data_spec)
{
	return TDI_NOT_SUPPORTED;
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntGetFirst(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		pipe_val_lookup_tbl_hdl_t tbl_hdl,
		pipe_tbl_match_spec_t *match_spec,
		pipe_data_spec_t *data_spec,
		pipe_val_lookup_ent_hdl_t *ent_hdl_p)
{
	return TDI_NOT_SUPPORTED;
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntGetNextNByKey(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		pipe_val_lookup_tbl_hdl_t tbl_hdl,
		pipe_tbl_match_spec_t *cur_match_spec,
		uint32_t n,
		pipe_tbl_match_spec_t *match_specs,
		pipe_data_spec_t **data_specs,
		uint32_t *num)
{
	return TDI_NOT_SUPPORTED;
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntGetFirstEntHandle(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		pipe_val_lookup_tbl_hdl_t tbl_hdl,
		pipe_val_lookup_ent_hdl_t *ent_hdl_p)
{
	return TDI_NOT_SUPPORTED;
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntGetNextEntHandle(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		pipe_val_lookup_tbl_hdl_t tbl_hdl,
		pipe_val_lookup_ent_hdl_t ent_hdl,
		uint32_t n,
		pipe_val_lookup_ent_hdl_t *nxt_ent_hdl)
{
	return TDI_NOT_SUPPORTED;
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatSpecToEntHdl(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		pipe_val_lookup_tbl_hdl_t tbl_hdl,
		pipe_tbl_match_spec_t *match_spec,
		pipe_val_lookup_ent_hdl_t *ent_hdl_p)
{
	return TDI_NOT_SUPPORTED;
}

pipe_status_t FixedFunctionMgrIntf::notificationRegister(
		dev_target_t dev_tgt,
		const char *table_name,
		fixed_func_mgr_update_callback cb,
		tdi_rt_attributes_type_e _attr_type,
		void *cb_cookie)
{
	return fixed_func_mgr_notification_register(dev_tgt,
                                                    table_name,
                                                    cb,
                                                    _attr_type,
                                                    cb_cookie);
}

pipe_status_t FixedFunctionMgrIntf::ffMgrMatEntStatsGet(
		pipe_sess_hdl_t sess_hdl,
		dev_target_t dev_tgt,
		const char *table_name,
		struct fixed_function_key_spec  *key_spec,
		struct fixed_function_data_spec *data_spec)
{
	return fixed_func_mgr_get_stats(sess_hdl,
                                        dev_tgt,
                                        table_name,
                                        key_spec,
                                        data_spec);
}

}  // namespace rt
}  // namespace pna
}  // namespace tdi
