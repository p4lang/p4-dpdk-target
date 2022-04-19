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

#include <memory>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <vector>

/* bf_rt_includes */

#ifdef __cplusplus
extern "C" {
#endif
#include <cjson/cJSON.h>
#ifdef __cplusplus
}
#endif
#include "bf_rt_cjson.hpp"

#include <target-sys/bf_sal/bf_sys_mem.h>

namespace bfrt {

CjsonObjHandler::CjsonObjHandler(const std::string &fileContent) {
  this->root = cJSON_Parse(fileContent.c_str());
  if (!this->root) {
    std::string error(cJSON_GetErrorPtr());
  }
}
CjsonObjHandler::~CjsonObjHandler() { cJSON_Delete(this->root); }

Cjson::Cjson(const Cjson &parent, const std::string &key) {
  root = cJSON_GetObjectItem(parent.root, key.c_str());
  this->cjson_mem_tracker = parent.cjson_mem_tracker;
}

Cjson Cjson::createCjsonFromFile(const std::string &fileContent) {
  Cjson obj;
  Cjson::createCjsonFromFileInternal(fileContent, obj);
  return obj;
}
void Cjson::createCjsonFromFileInternal(const std::string &fileContent,
                                        Cjson &obj) {
  obj.cjson_mem_tracker =
      std::shared_ptr<CjsonObjHandler>(new CjsonObjHandler(fileContent));
  obj.root = obj.cjson_mem_tracker->rootGet();
}

Cjson::Cjson(const Cjson &parent, int &index) {
  root = cJSON_GetArrayItem(parent.root, index);
  this->cjson_mem_tracker = parent.cjson_mem_tracker;
}

// Copy ctor
Cjson::Cjson(const Cjson &other) {
  this->root = other.root;
  this->cjson_mem_tracker = other.cjson_mem_tracker;
}
Cjson &Cjson::operator=(const Cjson &other) {
  this->root = other.root;
  this->cjson_mem_tracker = other.cjson_mem_tracker;
  return *this;
}

std::string Cjson::getCjsonKey() const {
  if (root && (root->type == cJSON_Object) && root->string) {
    return std::string(root->string);
  } else {
    return std::string("");
  }
}

std::vector<std::shared_ptr<Cjson>> Cjson::getCjsonChildVec() const {
  std::vector<std::shared_ptr<Cjson>> ret_vec;
  if (root) {
    int count = cJSON_GetArraySize(root);
    if (count != 0) {
      for (int i = 0; i < count; i++) {
        ret_vec.push_back(std::make_shared<Cjson>(*this, i));
      }
    }
  }
  return ret_vec;
}
std::vector<std::string> Cjson::getCjsonChildStringVec() const {
  std::vector<std::string> ret_vec;
  if (root) {
    int count = cJSON_GetArraySize(root);
    if (count != 0) {
      for (int i = 0; i < count; i++) {
        ret_vec.push_back(std::string(Cjson(*this, i)));
      }
    }
  }
  return ret_vec;
}
void Cjson::addObject(const std::string &name, const Cjson &item) {
  cJSON_AddItemReferenceToObject(this->root, name.c_str(), item.root);
}

uint32_t Cjson::array_size() { return cJSON_GetArraySize(root); }

Cjson::operator int() const {
  if (root && (root->type == cJSON_Number)) {
    return root->valueint;
  } else {
    // throw exception
    return 0;
  }
}
Cjson::operator unsigned int() const {
  if (root && (root->type == cJSON_Number)) {
    // Hack.. We should be using the appropriate datatype
    // using double here only because cJSON trims int to
    // INT_MAX if it exceeds signed range
    unsigned int ret_val = static_cast<unsigned int>(root->valuedouble);
    // double only has 52 bits of precision for value (rest is for
    // the exponent and the sign). So data right now for more than 52
    // bits of values are lost by cjson. hack to make it as all ffs if
    // more than 52 bit value. The uint64_t conversion will give 0 but
    // root->valuedouble will always be 18446744073709551615
    if (ret_val == 0 && root->valuedouble != 0) {
      ret_val = -1;
    }
    return ret_val;
  } else {
    return 0;
  }
}
Cjson::operator std::string() const {
  if (root && (root->type == cJSON_String)) {
    return std::string(root->valuestring);
  } else {
    return "";
  }
}
Cjson::operator bool() const {
  if (root && (root->type == cJSON_False)) {
    return false;
  } else if (root && (root->type == cJSON_True)) {
    return true;
  } else if (root) {
    return this->exists();
  } else {
    return false;
  }
}

Cjson::operator uint64_t() const {
  if (root && (root->type == cJSON_Number)) {
    // Hack.. We should be using the appropriate datatype
    // using double here only because cJSON trims int to
    // INT_MAX if it exceeds signed range
    uint64_t ret_val = static_cast<uint64_t>(root->valuedouble);
    if (ret_val == 0 && root->valuedouble != 0) {
      ret_val = -1;
    }
    return ret_val;
  } else {
    return 0;
  }
}

Cjson::operator float() const {
  if (root && (root->type == cJSON_Number)) {
    float ret_val = static_cast<float>(root->valuedouble);
    if (ret_val == 0 && root->valuedouble != 0) {
      ret_val = -1;
    }
    return ret_val;
  } else {
    return 0;
  }
}

Cjson Cjson::operator[](int index) const { return Cjson(*this, index); }

Cjson Cjson::operator[](const char *key) const {
  return Cjson(*this, std::string(key));
}
Cjson &Cjson::operator+=(const Cjson &other) {
  cJSON_AddItemReferenceToArray(this->root, other.root);
  return *this;
}

void Cjson::updateChildNode(const std::string &key, const std::string &val) {
  // remove key from this object(ex "name")
  cJSON_DeleteItemFromObject(this->root, key.c_str());
  // Now add a string object to this object with same key ("name")
  cJSON_AddStringToObject(this->root, key.c_str(), val.c_str());
}

std::ostream &operator<<(std::ostream &out, const Cjson &c) {
  auto cjson_out_str = cJSON_Print(c.root);
  if (cjson_out_str) {
      out << cjson_out_str << std::endl;
      bf_sys_free(cjson_out_str);
  }
  return out;
}

}  // namespace bfrt
