/*
 * Copyright(c) 2021 Intel Corporation.
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
#ifndef _BF_RT_PORT_TABLE_KEY_IMPL_HPP
#define _BF_RT_PORT_TABLE_KEY_IMPL_HPP

#include <bf_rt_common/bf_rt_table_key_impl.hpp>

namespace bfrt {

class BfRtPortCfgTableKey : public BfRtTableKeyObj {
 public:
  BfRtPortCfgTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), dev_port_(){};

  ~BfRtPortCfgTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  const uint32_t &getId() const { return dev_port_; }

  void setId(const uint32_t id) { dev_port_ = id; }

  bf_status_t reset() override final;

 private:
  uint32_t dev_port_;
};

class BfRtPortStatTableKey : public BfRtTableKeyObj {
 public:
  BfRtPortStatTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), dev_port_(){};

  ~BfRtPortStatTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  const uint32_t &getId() const { return dev_port_; }

  void setId(const uint32_t id) { dev_port_ = id; }

  bf_status_t reset() override final;

 private:
  uint32_t dev_port_;
};

class BfRtPortHdlInfoTableKey : public BfRtTableKeyObj {
 public:
  BfRtPortHdlInfoTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), conn_id_(), chnl_id_(){};

  ~BfRtPortHdlInfoTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  bf_status_t getPortHdl(uint32_t *conn_id, uint32_t *chnl_id) const;

  bf_status_t reset() override final;

  void setPortHdl(const uint32_t conn_id, const uint32_t chnl_id) {
    conn_id_ = conn_id;
    chnl_id_ = chnl_id;
  }

 private:
  uint32_t conn_id_;
  uint32_t chnl_id_;
};

class BfRtPortFpIdxInfoTableKey : public BfRtTableKeyObj {
 public:
  BfRtPortFpIdxInfoTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj), fp_idx_(){};
  ~BfRtPortFpIdxInfoTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const uint64_t &value);

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size);

  bf_status_t getValue(const bf_rt_id_t &field_id, uint64_t *value) const;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const;

  const uint32_t &getId() const { return fp_idx_; }

  void setId(const uint32_t idx) { fp_idx_ = idx; }

  bf_status_t reset() override final;

 private:
  uint32_t fp_idx_;
};

class BfRtPortStrInfoTableKey : public BfRtTableKeyObj {
 public:
  BfRtPortStrInfoTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};
  ~BfRtPortStrInfoTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id, const std::string &value);

  bf_status_t getValue(const bf_rt_id_t &field_id, std::string *value) const;

  const std::string &getPortStr() const { return port_str_; };

  void setPortStr(const std::string &name_str) { port_str_ = name_str; };

 private:
  std::string port_str_;
};

}  // bfrt
#endif  // _BF_RT_PORT_TABLE_KEY_IMPL_HPP
