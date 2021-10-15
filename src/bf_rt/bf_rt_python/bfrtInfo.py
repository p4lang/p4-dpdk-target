#
# Copyright(c) 2021 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
from __future__ import print_function
from ctypes import *
import pdb

class BfRtInfo:

    """
    This class contains abstractions on the BF Runtime C API for all
    tables described by a BF Runtime Info object. This includes infos
    for both p4 programs and for pd_fixed APIs exposed through
    BF Runtime.
    """
    def __init__(self, cintf, name):
        self._cintf = cintf
        self.name = name
        self.tbl_id_map = {}
        self.tbl_dep_map = {}
        self.lrn_id_map = {}
        self.nested_tables = []
        sts = self._init_handle()
        if not sts == 0:
            print("BfRtInfo init hanle failed for {}!".format(self.name))
            return -1
        sts = self._init_tables()
        if not sts == 0:
            print("BfRtInfo init tables failed for {}!".format(self.name))
            return -1

    def _init_handle(self):
        self._handle = self._cintf.handle_type()
        sts = self._cintf.get_driver().bf_rt_info_get(self._cintf.get_dev_id(), self.name, byref(self._handle))
        if not sts == 0:
            print("CLI Error: get info failed for {}".format(self.name))
        return sts

    def _init_tables(self):
        num_tables = c_int(-1)
        sts = self._cintf.get_driver().bf_rt_num_tables_get(self._handle, byref(num_tables))
        if not sts == 0:
            print("CLI Error: get num tables for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
            return sts
        array_type = self._cintf.handle_type * num_tables.value
        tables = array_type()
        sts = self._cintf.get_driver().bf_rt_tables_get(self._handle, tables)
        if not sts == 0:
            print("CLI Error: get table handles for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
            return sts
        # Python Tables Object Initialzation
        # print("{:40s} | {:30s} | {:10s}".format("TableName","Table Type","Status"))
        self.tables = {}
        for table in tables:
            tbl_obj = self._cintf.BfRtTable(self._cintf, table, self)
            if tbl_obj == -1:
                print("CLI Error: bad table object init")
                return -1
            elif tbl_obj.table_type_map(tbl_obj.get_type()) == "INVLD":
                print("CLI Error: bad table type init")
                return -1
            else:
                tbl_id = c_uint(0)
                sts = self._cintf.get_driver().bf_rt_table_id_from_handle_get(table, byref(tbl_id))
                tbl_obj.set_id(tbl_id.value)
                has_const_default_action = c_bool(False)
                sts = self._cintf.get_driver().bf_rt_table_has_const_default_action(table,
                        byref(has_const_default_action))
                tbl_obj.set_has_const_default_action(has_const_default_action)
                self.tables[tbl_obj.name] = tbl_obj
                self.tbl_id_map[tbl_id.value] = tbl_obj
        # Tables Dependencies Initialzation
        for tbl_id, tbl_obj in self.tbl_id_map.items():
            num_deps = c_int()
            self._cintf.get_driver().bf_rt_num_tables_this_table_depends_on_get(self._handle, tbl_id, byref(num_deps))
            if num_deps.value == 0:
                continue
            array_type = c_uint * num_deps.value
            deps = array_type()
            self._cintf.get_driver().bf_rt_tables_this_table_depends_on_get(self._handle, tbl_id, deps)
            self.tbl_dep_map[tbl_id] = deps
            # Nested tables are to be included in the parent table from depends_on field
            if tbl_obj.table_type in self.nested_tables:
                prefix = self.tbl_id_map[deps[0]].name
                # New name is used to create node tree
                new_name = prefix + tbl_obj.name[tbl_obj.name.rfind('.'):]
                self.tables[new_name] = tbl_obj
                del self.tables[tbl_obj.name]
                tbl_obj.name = new_name

        return 0

    def _init_learns(self):
        num_learns = c_int(-1)
        sts = self._cintf.get_driver().bf_rt_num_learns_get(self._handle, byref(num_learns))
        if not sts == 0:
            print("CLI Error: get num learns for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
            return sts
        array_type = self._cintf.handle_type * num_learns.value
        learns = array_type()
        sts = self._cintf.get_driver().bf_rt_learns_get(self._handle, learns)
        if not sts == 0:
            print("CLI Error: get learn handles for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
            return sts
        self.learns = {}
        for learn in learns:
            lrn_obj = self._cintf.BfRtLearn(self._cintf, learn, self)
            if lrn_obj == -1:
                return -1
            lrn_id = c_uint(0)
            sts = self._cintf.get_driver().bf_rt_learn_id_get(learn, byref(lrn_id))
            lrn_obj.set_id(lrn_id.value)
            self.learns[lrn_obj.name] = lrn_obj
            self.lrn_id_map[lrn_id.value] = lrn_obj
        return 0
