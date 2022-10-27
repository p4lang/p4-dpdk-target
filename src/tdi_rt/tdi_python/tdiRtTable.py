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
from tdiTable import *
from tdiRtDefs import *
import functools

def target_check_and_set(f):
    @functools.wraps(f)
    def target_wrapper(*args, **kw):
        cintf = args[0]._cintf
        pipe = None
        gress_dir = None
        old_tgt = None

        for k,v in kw.items():
            if k == "pipe":
                pipe = v
            elif k == "gress_dir":
                gress_dir = v

        if pipe is not None or gress_dir is not None:
            old_tgt = cintf._dev_tgt
        if pipe is not None:
            cintf._set_pipe(pipe=pipe)
        if gress_dir is not None:
            cintf._set_direction(gress_dir)
        # If there is an exception, then revert back the
        # target. Store the ret value of the original function
        # in case it does return something like some entry_get
        # functions and return it at the end
        ret_val = None
        try:
            ret_val = f(*args, **kw)
        except Exception as e:
            if old_tgt:
                cintf._dev_tgt = old_tgt
            raise e
        if old_tgt:
            cintf._dev_tgt = old_tgt
        return ret_val
    return target_wrapper

class TdiRtTableEntry(TableEntry):
    @target_check_and_set
    def push(self, verbose=False, pipe=None, gress_dir=None):
        TableEntry.push(self, verbose)

    @target_check_and_set
    def update(self, pipe=None, gress_dir=None):
        TableEntry.update(self)

    @target_check_and_set
    def remove(self, pipe=None, gress_dir=None):
        TableEntry.remove(self)

class TdiRtTable(TdiTable):
    key_match_type_cls = KeyMatchTypeRt
    table_type_cls = TableTypeRt
    attributes_type_cls = AttributesTypeRt
    operations_type_cls = OperationsTypeRt
    flags_type_cls = FlagsTypeRt
    table_entry_cls = TdiRtTableEntry

    """
        Remove $ and change the table names to lower case
    """
    def modify_table_names(self, table_name):
        self.name = table_name.value.decode('ascii')
        #Unify the table name for TDINode (command nodes)
        name_lowercase_without_dollar=self.name.lower().replace("$","")
        if self.table_type in ["PORT_CFG", "PORT_STAT", "PORT_HDL_INFO", "PORT_FRONT_PANEL_IDX_INFO", "PORT_STR_INFO"]:
            self.name = "port.{}".format(name_lowercase_without_dollar)
        if self.table_type in ["PRE_MGID", "PRE_NODE", "PRE_ECMP", "PRE_LAG", "PRE_PRUNE", "PRE_PORT", "MIRROR_CFG"]:
            self.name = name_lowercase_without_dollar
        if self.table_type in ["TM_PORT_GROUP_CFG", "TM_PORT_GROUP"]:
            self.name = name_lowercase_without_dollar
        if self.table_type in ["SNAPSHOT", "SNAPSHOT_LIVENESS"]:
            self.name = "{}".format(name_lowercase_without_dollar)
        if self.table_type in ["FIXED_FUNC"]:
            self.name = "fixed.{}".format(name_lowercase_without_dollar)
