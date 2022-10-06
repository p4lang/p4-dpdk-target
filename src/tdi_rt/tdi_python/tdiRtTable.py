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

class TdiRtTable(TdiTable):
    key_match_type_cls = KeyMatchTypeRt
    table_type_cls = TableTypeRt
    attributes_type_cls = AttributesTypeRt
    operations_type_cls = OperationsTypeRt
    flags_type_cls = FlagsTypeRt

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
