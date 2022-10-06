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
#ifndef _TDI_FIXED_TABLE_DATA_IMPL_HPP
#define _TDI_FIXED_TABLE_DATA_IMPL_HPP

#include <tdi/common/tdi_table.hpp>
#include <tdi/common/tdi_table_data.hpp>

#include <tdi_common/tdi_context_info.hpp>

namespace tdi {
namespace pna {
namespace rt {
// FixedFunctionConfigTable ******************
class FixedFunctionTableDataSpec {
 public:
        FixedFunctionTableDataSpec() {}

	FixedFunctionTableDataSpec(const size_t &data_sz) {
		std::memset(&fixed_spec_data, 0, sizeof(fixed_spec_data));
		data_bytes.reset(new uint8_t[data_sz]());
		fixed_spec_data.data_bytes = data_bytes.get();
		fixed_spec_data.num_data_bytes = data_sz;
	};

        tdi_status_t setFixedFunctionData(const tdi::DataFieldInfo &field,
                                          const uint64_t &value,
                                          const uint8_t *value_ptr,
                                          const std::string &value_str,
                                          pipe_data_spec_t *fixed_spec_data);

	void getFixedFunctionData(fixed_function_data_spec_t *value,
                                  pipe_data_spec_t *spec_data) const;

        void getFixedFunctionDataBytes(const tdi::DataFieldInfo &field,
                                       const size_t size,
			               uint8_t *value,
			               pipe_data_spec_t *fixed_spec_data) const;

        void getFixedFunctionDataSpec(pipe_data_spec_t *value,
                                      pipe_data_spec_t *fixed_spec_data) const;

	pipe_data_spec_t *getFixedFunctionDataSpec()  {
		return &fixed_spec_data;
	}

	const pipe_data_spec_t *getFixedFunctionDataSpec() const {
		return &fixed_spec_data;
	}

	void reset() {
		std::memset(&fixed_spec_data, 0, sizeof(fixed_spec_data));
	}

        ~FixedFunctionTableDataSpec() {}

        pipe_data_spec_t fixed_spec_data {0};
        std::unique_ptr<uint8_t[]> data_bytes{nullptr};
};

class FixedFunctionTableData : public tdi::TableData {
 public:
  FixedFunctionTableData(const tdi::Table *table,
                         tdi_id_t act_id,
                         const std::vector<tdi_id_t> &fields)
      : tdi::TableData(table, act_id, fields) {
    size_t data_sz = 0;

    data_sz = static_cast<const RtTableContextInfo *>(
                    this->table_->tableInfoGet()->tableContextInfoGet())
                    ->maxDataSzGet();
    fixed_spec_data_ = new FixedFunctionTableDataSpec(data_sz);
  }

  ~FixedFunctionTableData() {
	  if (fixed_spec_data_)
		  delete(fixed_spec_data_);
  }

  FixedFunctionTableData(const tdi::Table *tbl_obj,
                         const std::vector<tdi_id_t> &fields)
      : FixedFunctionTableData(tbl_obj, 0, fields) {}

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const std::string &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override;

  tdi_status_t setValue(const tdi_id_t &field_id, const float &value) override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::string *value) const override;
  tdi_status_t getValue(const tdi_id_t &field_id, float *value) const override;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        std::vector<uint64_t> *value) const override;

  tdi_status_t resetDerived();
  tdi_status_t reset(const tdi_id_t &action_id,
                     const tdi_id_t &/*container_id*/,
                     const std::vector<tdi_id_t> &fields);

  const uint32_t &numActionOnlyFieldsGet() const {
    return num_action_only_fields;
  }
  pipe_act_fn_hdl_t getActFnHdl() const;

  // Functions not exposed
  const FixedFunctionTableDataSpec &getFixedFunctionTableDataSpecObj() const {
	  return *fixed_spec_data_;
  }
  FixedFunctionTableDataSpec &getFixedFunctionTableDataSpecObj() {
	  return *fixed_spec_data_;
  }
  const FixedFunctionTableDataSpec &getFixedFunctionTableDataSpecObj_() const {
	  return *(spec_data_wraper.get());
  }
  FixedFunctionTableDataSpec &getFixedFunctionTableDataSpecObj_() {
	  return *(spec_data_wraper.get());
  }

  FixedFunctionTableDataSpec *fixed_spec_data_;

 protected:
  std::unique_ptr<FixedFunctionTableDataSpec> spec_data_wraper;

 private:
  tdi_status_t setValueInternal(const tdi_id_t &field_id,
                                const uint64_t &value,
                                const uint8_t *value_ptr,
                                const size_t &s,
                                const std::string &str_value);

  tdi_status_t getValueInternal(const tdi_id_t &field_id,
                                uint64_t *value,
                                uint8_t *value_ptr,
                                const size_t &size) const;

  void initializeDataFields();

  uint32_t num_action_only_fields = 0;
};

}  // namespace rt
}  // namespace pna
}  // namespace tdi

#endif  // _TDI_FIXED_TABLE_DATA_IMPL_HPP
