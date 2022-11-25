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
#include <unordered_set>

// tdi include
#include <tdi/common/tdi_init.hpp>

// local rt includes
#include "../tdi_common/tdi_fixed_mgr_intf.hpp"
#include <tdi_common/tdi_rt_target.hpp>
#include <tdi_fixed/tdi_fixed_table_impl.hpp>
#include <tdi_fixed/tdi_fixed_table_key_impl.hpp>
#include <tdi_fixed/tdi_fixed_table_data_impl.hpp>

namespace tdi {
namespace pna {
namespace rt {
namespace {

template <class Table, class Key>
tdi_status_t key_reset(const Table & /*table*/, Key *match_key) {
  return match_key->reset();
}

}  // anonymous namespace

// FixedFunctionConfigTable ******************
tdi_status_t FixedFunctionConfigTable::defaultEntryGet(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags &flags,
		tdi::TableData *data) const {
	return entryGet_internal(session, dev_tgt, flags, nullptr, data);
}

tdi_status_t FixedFunctionConfigTable::entryAdd(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags & /*flags*/,
		const tdi::TableKey &key,
		const tdi::TableData &data) const {
  auto *fixedFuncMgr  = FixedFunctionMgrIntf::getInstance();
  const FixedFunctionTableKey &match_key =
	  static_cast<const FixedFunctionTableKey &>(key);
  const FixedFunctionTableData &match_data =
      static_cast<const FixedFunctionTableData &>(data);

  if (this->tableInfoGet()->isConst()) {
    LOG_DBG(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  fixed_function_key_spec_t fixed_match_spec = {0};
  match_key.populate_fixed_fun_key_spec(&fixed_match_spec);
  dev_target_t fixed_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&fixed_dev_tgt, nullptr);

  fixed_function_data_spec_t data_value;
  match_data.getFixedFunctionTableDataSpecObj().
	  getFixedFunctionData(&data_value,
                               &match_data.fixed_spec_data_->fixed_spec_data);

  tdi_status_t status =  fixedFuncMgr->ffMgrMatEntAdd(
		  session.handleGet(
		     static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
		  fixed_dev_tgt,
		  tableInfoGet()->nameGet().c_str(),
		  &fixed_match_spec,
		  &data_value);

  if (status != TDI_SUCCESS) {
    LOG_DBG("%s:%d Entry add failed for table %s, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t fixed_dataReset_internal(const tdi::Table & /*table*/,
                                      const tdi_id_t & /*action_id*/,
                                      const std::vector<tdi_id_t> & /*fields*/,
                                      tdi::TableData * /*data*/) {
	return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::dataReset(tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return fixed_dataReset_internal(*this, 0, fields, data);
}

tdi_status_t FixedFunctionConfigTable::dataReset(const tdi_id_t &action_id,
   		                                 tdi::TableData *data) const {
  std::vector<tdi_id_t> fields;
  return fixed_dataReset_internal(*this, action_id, fields, data);
}

tdi_status_t FixedFunctionConfigTable::dataReset(
		const std::vector<tdi_id_t> &fields,
		tdi::TableData *data) const {
  return fixed_dataReset_internal(*this, 0, fields, data);
}

tdi_status_t FixedFunctionConfigTable::dataReset(
		const std::vector<tdi_id_t> &fields,
		const tdi_id_t &action_id,
		tdi::TableData *data) const {
  return fixed_dataReset_internal(*this, action_id, fields, data);
}

tdi_status_t FixedFunctionConfigTable::entryMod(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags &flags,
		const tdi::TableKey &key,
		const tdi::TableData &data) const {
  return TDI_NOT_SUPPORTED;
}

tdi_status_t FixedFunctionConfigTable::keyAllocate(
    std::unique_ptr<TableKey> *key_ret) const {
  *key_ret = std::unique_ptr<TableKey>(new FixedFunctionTableKey(this));
  if (*key_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::keyReset(TableKey *key) const {
  FixedFunctionTableKey *match_key = static_cast<FixedFunctionTableKey *>(key);
  return key_reset<FixedFunctionConfigTable, FixedFunctionTableKey>(*this,
			match_key);
}

tdi_status_t FixedFunctionConfigTable::dataAllocate(
    std::unique_ptr<tdi::TableData> *data_ret) const {
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t FixedFunctionConfigTable::dataAllocate(
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  // Create a empty vector to indicate all fields are needed
  std::vector<tdi_id_t> fields;
  return dataAllocate_internal(action_id, data_ret, fields);
}

tdi_status_t FixedFunctionConfigTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(0, data_ret, fields);
}

tdi_status_t FixedFunctionConfigTable::dataAllocate(
    const std::vector<tdi_id_t> &fields,
    const tdi_id_t &action_id,
    std::unique_ptr<tdi::TableData> *data_ret) const {
  return dataAllocate_internal(action_id, data_ret, fields);
}

tdi_status_t FixedFunctionConfigTable::clear(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags & /*flags*/) const {
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::usageGet(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags &flags,
		uint32_t *count) const {
	return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::dataAllocate_internal(
		tdi_id_t action_id,
		std::unique_ptr<tdi::TableData> *data_ret,
		const std::vector<tdi_id_t> &fields) const {
  *data_ret = std::unique_ptr<tdi::TableData>(
      new FixedFunctionTableData(this, action_id, fields));
  if (*data_ret == nullptr) {
    return TDI_NO_SYS_RESOURCES;
  }
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::sizeGet(const tdi::Session &session,
                                               const tdi::Target &dev_tgt,
                                               const tdi::Flags & /*flags*/,
                                               size_t *count) const {
  *count = this->tableInfoGet()->sizeGet();
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::entryGetFirst(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags &flags,
		tdi::TableKey *key,
		tdi::TableData *data) const {
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::entryKeyGet(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags & /*flags*/,
		const tdi_handle_t &entry_handle,
		tdi::Target *entry_tgt,
		tdi::TableKey *key) const {
  return TDI_SUCCESS;
}

tdi_status_t FixedFunctionConfigTable::entryDel(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags & /*flags*/,
		const tdi::TableKey &key) const {
  auto *fixedFuncMgr = FixedFunctionMgrIntf::getInstance();
  const FixedFunctionTableKey &match_key =
	  static_cast<const FixedFunctionTableKey &>(key);

  if (this->tableInfoGet()->isConst()) {
    LOG_DBG(
        "%s:%d %s Cannot perform this API because table has const entries",
        __func__,
        __LINE__,
        this->tableInfoGet()->nameGet().c_str());
    return TDI_INVALID_ARG;
  }

  fixed_function_key_spec_t fixed_match_spec = {0};

  match_key.populate_fixed_fun_key_spec(&fixed_match_spec);
  dev_target_t fixed_dev_tgt;
  auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
  dev_target->getTargetVals(&fixed_dev_tgt, nullptr);

  tdi_status_t status = fixedFuncMgr->ffMgrMatEntDel(
		  session.handleGet(
			static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
		  fixed_dev_tgt,
		  tableInfoGet()->nameGet().c_str(),
		  &fixed_match_spec);

  if (status != TDI_SUCCESS) {
    LOG_DBG("%s:%d Entry delete failed for table %s, err %d",
              __func__,
              __LINE__,
              tableInfoGet()->nameGet().c_str(),
              status);
  }
  return status;
}

tdi_status_t FixedFunctionConfigTable::entryGet_internal(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags &flags,
		fixed_function_key_spec_t *pipe_match_spec,
                tdi::TableData *data) const {
    auto *fixedFuncMgr  = FixedFunctionMgrIntf::getInstance();
    tdi_status_t status = TDI_NOT_SUPPORTED;
    fixed_function_data_spec_t data_spec;
    std::vector<tdi_id_t> dataFields;
    dev_target_t fixed_dev_tgt;

    auto dev_target = static_cast<const tdi::pna::rt::Target *>(&dev_tgt);
    dev_target->getTargetVals(&fixed_dev_tgt, nullptr);

    FixedFunctionTableData *match_data =
	    static_cast<FixedFunctionTableData *>(data);

    // reset the data object with act_id 0 and all fields
    match_data->TableData::reset(0);

    match_data->getFixedFunctionTableDataSpecObj().
	    getFixedFunctionData(&data_spec,
			         &match_data->fixed_spec_data_->fixed_spec_data);
    /* TODO - add regular key-data table support as required */
    if (!pipe_match_spec) {
	    //key-less table
	    status = fixedFuncMgr->ffMgrMatEntGetDefaultEntry(
			session.handleGet(
		        static_cast<tdi_mgr_type_e>(TDI_RT_MGR_TYPE_PIPE_MGR)),
			fixed_dev_tgt,
			tableInfoGet()->nameGet().c_str(),
			&data_spec);

	    if (status != TDI_SUCCESS) {
	        LOG_TRACE(
	        "%s:%d %s ERROR getting defualt entry for fixed function, "
		"err %d",
		__func__,
		__LINE__,
		tableInfoGet()->nameGet().c_str(),
		status);
		return status;
	    }

	    dataFields = tableInfoGet()->dataFieldIdListGet(0);
	    match_data->activeFieldsSet(dataFields);
    }

    return status;
}

tdi_status_t FixedFunctionConfigTable::entryGet(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags &flags,
		const tdi::TableKey &key,
		tdi::TableData *data) const {
       return TDI_NOT_SUPPORTED;
}

tdi_status_t FixedFunctionConfigTable::entryGet(
		const tdi::Session &session,
		const tdi::Target &dev_tgt,
		const tdi::Flags &flags,
		const tdi_handle_t &entry_handle,
		tdi::TableKey *key,
		tdi::TableData *data) const {
  return TDI_NOT_SUPPORTED;
}


tdi_status_t FixedFunctionConfigTable::entryHandleGet(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags & /*flags*/,
    const tdi::TableKey &key,
    tdi_handle_t *entry_handle) const {
  return TDI_NOT_SUPPORTED;
}

tdi_status_t FixedFunctionConfigTable::entryGetNextN(
    const tdi::Session &session,
    const tdi::Target &dev_tgt,
    const tdi::Flags &flags,
    const tdi::TableKey &key,
    const uint32_t &n,
    tdi::Table::keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  return TDI_NOT_SUPPORTED;
}

}  // namespace rt
}  // namespace pna
}  // namespace tdi
