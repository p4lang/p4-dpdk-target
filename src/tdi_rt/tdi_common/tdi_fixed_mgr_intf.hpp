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

#ifndef _TDI_FIXED_MGR_INTERFACE_HPP
#define _TDI_FIXED_MGR_INTERFACE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_mem.h>

#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/shared/pipe_mgr_fixed.h>
#include <dvm/bf_drv_intf.h>
#ifdef __cplusplus
}
#endif

#include <map>
#include <iostream>
#include <mutex>
#include <memory>

/* tdi_includes */
#include "tdi_session_impl.hpp"
#include <tdi/common/tdi_utils.hpp>
#include <tdi/common/tdi_defs.h>

namespace tdi {
namespace pna {
namespace rt {

class IFixedFunctionMgrIntf {
 public:
  virtual ~IFixedFunctionMgrIntf() = default;

  // APIs for Fixed function Manipulation
  virtual pipe_status_t ffMgrMatEntAdd(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  const char *name,
		  fixed_function_key_spec *match_spec,
		  fixed_function_data_spec *data_spec) = 0;

  virtual pipe_status_t ffMgrMatEntDel(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  const char *name,
		  struct fixed_function_key_spec *match_spec) = 0;

  virtual pipe_status_t ffMgrMatEntGet(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_val_lookup_ent_hdl_t ent_hdl,
		  pipe_tbl_match_spec_t *match_spec,
		  pipe_data_spec_t *data_spec) = 0;

  virtual pipe_status_t ffMgrMatEntGetFirst(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_tbl_match_spec_t *match_spec,
		  pipe_data_spec_t *data_spec,
		  pipe_val_lookup_ent_hdl_t *ent_hdl_p) = 0;

  virtual pipe_status_t ffMgrMatEntGetNextNByKey(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_tbl_match_spec_t *cur_match_spec,
		  uint32_t n,
		  pipe_tbl_match_spec_t *match_specs,
		  pipe_data_spec_t **data_specs,
		  uint32_t *num) = 0;

  virtual pipe_status_t ffMgrMatEntGetFirstEntHandle(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_val_lookup_ent_hdl_t *ent_hdl_p) = 0;

  virtual pipe_status_t ffMgrMatEntGetNextEntHandle(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_val_lookup_ent_hdl_t ent_hdl,
		  uint32_t n,
		  pipe_val_lookup_ent_hdl_t *nxt_ent_hdl) = 0;

    virtual pipe_status_t ffMgrMatSpecToEntHdl(
		    pipe_sess_hdl_t sess_hdl,
		    dev_target_t dev_tgt,
		    pipe_val_lookup_tbl_hdl_t tbl_hdl,
		    pipe_tbl_match_spec_t *match_spec,
		    pipe_val_lookup_ent_hdl_t *ent_hdl_p) = 0;

 protected:
  static std::unique_ptr<IFixedFunctionMgrIntf> instance;
  static std::mutex fixed_mgr_intf_mtx;
};

class FixedFunctionMgrIntf : public IFixedFunctionMgrIntf{
 public:
  virtual ~FixedFunctionMgrIntf() {
    if (instance) {
      instance.release();
    }
  };
  FixedFunctionMgrIntf() = default;

  // To be used only with API that are not session bound.
  static IFixedFunctionMgrIntf *getInstance() {
    if (instance.get() == nullptr) {
      fixed_mgr_intf_mtx.lock();
      if (instance.get() == nullptr) {
        instance.reset(new FixedFunctionMgrIntf());
      }
      fixed_mgr_intf_mtx.unlock();
    }
    return IFixedFunctionMgrIntf::instance.get();
  }

  // APIs for Fixed function Manipulation
  pipe_status_t ffMgrMatEntAdd(pipe_sess_hdl_t sess_hdl,
                               dev_target_t dev_tgt,
                               const char *name,
                               fixed_function_key_spec *match_spec,
                               fixed_function_data_spec *data_spec);

  pipe_status_t ffMgrMatEntDel(pipe_sess_hdl_t sess_hdl,
                               dev_target_t dev_tgt,
                               const char *name,
			       struct fixed_function_key_spec *match_spec);

  pipe_status_t ffMgrMatEntGet(pipe_sess_hdl_t sess_hdl,
		               dev_target_t dev_tgt,
			       pipe_val_lookup_tbl_hdl_t tbl_hdl,
			       pipe_val_lookup_ent_hdl_t ent_hdl,
			       pipe_tbl_match_spec_t *match_spec,
			       pipe_data_spec_t *data_spec);

  pipe_status_t ffMgrMatEntGetFirst(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_tgt,
				    pipe_val_lookup_tbl_hdl_t tbl_hdl,
				    pipe_tbl_match_spec_t *match_spec,
				    pipe_data_spec_t *data_spec,
				    pipe_val_lookup_ent_hdl_t *ent_hdl_p);

  pipe_status_t ffMgrMatEntGetNextNByKey(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_tbl_match_spec_t *cur_match_spec,
		  uint32_t n,
		  pipe_tbl_match_spec_t *match_specs,
		  pipe_data_spec_t **data_specs,
		  uint32_t *num);

  pipe_status_t ffMgrMatEntGetFirstEntHandle(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_val_lookup_ent_hdl_t *ent_hdl_p);

  pipe_status_t ffMgrMatEntGetNextEntHandle(
		  pipe_sess_hdl_t sess_hdl,
		  dev_target_t dev_tgt,
		  pipe_val_lookup_tbl_hdl_t tbl_hdl,
		  pipe_val_lookup_ent_hdl_t ent_hdl,
		  uint32_t n,
		  pipe_val_lookup_ent_hdl_t *nxt_ent_hdl);

    pipe_status_t ffMgrMatSpecToEntHdl(
		    pipe_sess_hdl_t sess_hdl,
		    dev_target_t dev_tgt,
		    pipe_val_lookup_tbl_hdl_t tbl_hdl,
		    pipe_tbl_match_spec_t *match_spec,
		    pipe_val_lookup_ent_hdl_t *ent_hdl_p);

 private:
  FixedFunctionMgrIntf(const FixedFunctionMgrIntf &src) = delete;
  FixedFunctionMgrIntf &operator=(const FixedFunctionMgrIntf &rhs) = delete;
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // _TDI_FIXED_MGR_INTERFACE_HPP
