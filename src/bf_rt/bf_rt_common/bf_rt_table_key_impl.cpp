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

//#include <bitset>
//#include <arpa/inet.h>
//#include <inttypes.h>

#include "bf_rt_table_key_impl.hpp"
#include "bf_rt_table_impl.hpp"
#include "bf_rt_utils.hpp"

namespace bfrt {

// BASE BfRtTableKeyObj *****************
bf_status_t BfRtTableKeyObj::setValue(const bf_rt_id_t & /*field_id*/,
                                      const uint64_t & /*value*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValue(const bf_rt_id_t & /*field_id*/,
                                      const uint8_t * /*value*/,
                                      const size_t & /*size*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValue(const bf_rt_id_t & /* field_id */,
                                      const std::string & /* string */) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValueandMask(const bf_rt_id_t & /*field_id*/,
                                             const uint64_t & /*value*/,
                                             const uint64_t & /*mask*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValueandMask(const bf_rt_id_t & /*field_id*/,
                                             const uint8_t * /*value1*/,
                                             const uint8_t * /*value2*/,
                                             const size_t & /*size*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValueRange(const bf_rt_id_t & /*field_id*/,
                                           const uint64_t & /*start*/,
                                           const uint64_t & /*end*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValueRange(const bf_rt_id_t & /*field_id*/,
                                           const uint8_t * /*start*/,
                                           const uint8_t * /*end*/,
                                           const size_t & /*size*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValueLpm(const bf_rt_id_t & /*field_id*/,
                                         const uint64_t & /*value1*/,
                                         const uint16_t & /*p_length*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValueLpm(const bf_rt_id_t & /*field_id*/,
                                         const uint8_t * /*value1*/,
                                         const uint16_t & /*p_length*/,
                                         const size_t & /*size*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}
bf_status_t BfRtTableKeyObj::setValueOptional(const bf_rt_id_t & /*field_id*/,
                                              const uint64_t & /*value*/,
                                              const bool & /*is_valid*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::setValueOptional(const bf_rt_id_t & /*field_id*/,
                                              const uint8_t * /*value*/,
                                              const bool & /*is_valid*/,
                                              const size_t & /*size*/) {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValue(const bf_rt_id_t & /*field_id*/,
                                      uint64_t * /*value*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValue(const bf_rt_id_t & /*field_id*/,
                                      const size_t & /*size*/,
                                      uint8_t * /*value*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValue(const bf_rt_id_t & /*field_id*/,
                                      std::string * /*value*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValueandMask(const bf_rt_id_t & /*field_id*/,
                                             uint64_t * /*\alue1*/,
                                             uint64_t * /*value2*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValueandMask(const bf_rt_id_t & /*field_id*/,
                                             const size_t & /*size*/,
                                             uint8_t * /*value1*/,
                                             uint8_t * /*value2*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValueRange(const bf_rt_id_t & /*field_id*/,
                                           uint64_t * /*start*/,
                                           uint64_t * /*end*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValueRange(const bf_rt_id_t & /*field_id*/,
                                           const size_t & /*size*/,
                                           uint8_t * /*start*/,
                                           uint8_t * /*end*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValueLpm(const bf_rt_id_t & /*field_id*/,
                                         uint64_t * /*start*/,
                                         uint16_t * /*pLength*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValueLpm(const bf_rt_id_t & /*field_id*/,
                                         const size_t & /*size*/,
                                         uint8_t * /*value1*/,
                                         uint16_t * /*p_length*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}
bf_status_t BfRtTableKeyObj::getValueOptional(const bf_rt_id_t & /*field_id*/,
                                              uint64_t * /*value*/,
                                              bool * /*is_valid*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::getValueOptional(const bf_rt_id_t & /*field_id*/,
                                              const size_t & /*size*/,
                                              uint8_t * /*value*/,
                                              bool * /*is_valid*/) const {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

bf_status_t BfRtTableKeyObj::tableGet(const BfRtTable **table) const {
  *table = table_;
  return BF_SUCCESS;
}

bf_status_t BfRtTableKeyObj::reset() {
  LOG_ERROR("%s:%d Not supported", __func__, __LINE__);
  return BF_NOT_SUPPORTED;
}

}  // bfrt
