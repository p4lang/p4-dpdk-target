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
from traitlets.config import Config
from prompt_toolkit.input.defaults import create_input
from prompt_toolkit.output.defaults import create_output
from prompt_toolkit.application.current import get_app_session
from ctypes import *
import sys
import os
import builtins
import keyword
import IPython
import types
import tabulate
import pdb
import re
import atexit
import json
import operator
import copy
import functools

#sys.path.append('../../../third-party/tdi/tdi_python/')
from tdicli import *
from tdiInfo import *
from tdiRtTable import *

"""
    TdiRtLeaf is derived from TDILeaf so that we can generate target specific dynamic APIs
"""
class TdiRtLeaf(TDILeaf):
    TDILeaf.target_check_and_set = target_check_and_set

    # Generating add_with_<action> APIs dynamically
    def _create_add_with_action(self, key_fields, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None):
    """Add entry to {} table with action: {}

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}",{}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return

    self._c_tbl.add_entry(parsed_keys, parsed_data, b'{}')
        '''
        TDILeaf._create_add_with_action(self, key_fields, data_fields, action_name, code_str)

    # Generating add APIs dynamically
    def _create_add(self, key_fields, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None):
    """Add entry to {} table.

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.add_entry(parsed_keys, parsed_data)
        '''
        TDILeaf._create_add(self, key_fields, data_fields, code_str)

    # Generating set_default_with_<action> APIs dynamically
    def _create_set_default_with_action(self, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None):
    """Set default action for {} table with action: {}

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.set_default_entry(parsed_data, b'{}')
        '''
        TDILeaf._create_set_default_with_action(self, data_fields, action_name, code_str)

    # Generating set_default API dynamically
    def _create_set_default(self, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None):
    """Set default action for {} table.

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.set_default_entry(parsed_data)
        '''
        TDILeaf._create_set_default(self, data_fields, code_str)

    # Generating reset_default API dynamically
    def _create_reset_default(self):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, pipe=None, gress_dir=None):
    """Set default action for {} table.
    """
    self._c_tbl.reset_default_entry()
        '''
        TDILeaf._create_reset_default(self, code_str)

    # Generating mod_with_<action> APIs dynamically
    def _create_mod_with_action(self, key_fields, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, ttl_reset=True):
    """Modify entry in {} table using action: {}

    Parameters:
    {}
    ttl_reset: default=True, set to False in order to not start aging entry from the beggining.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.mod_entry(parsed_keys, parsed_data, b'{}', ttl_reset=ttl_reset)'''
        TDILeaf._create_mod_with_action(self, key_fields, data_fields, action_name, code_str)

    # Generating mod_inc_with_<action> APIs dynamically
    def _create_mod_inc_with_action(self, key_fields, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} mod_flag=0, pipe=None, gress_dir=None):
    """ Incremental Add/Delete items in the fields that are array for the matched entry in {} table.

    Parameters:
    {}
    mod_flag: 0/1 value for Add/Delete Item in the field that is array type.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.mod_inc_entry(parsed_keys, parsed_data, mod_flag, b'{}')
        '''
        TDILeaf._create_mod_inc_with_action(self, key_fields, data_fields, action_name, code_str)

    # Generating mod API dynamically
    def _create_mod(self, key_fields, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, ttl_reset=True):
    """Modify any field in the entry at once in {} table.

    Parameters:
    {}
    ttl_reset: default=True, set to False in order to not start aging entry from the beggining.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.mod_entry(parsed_keys, parsed_data, ttl_reset=ttl_reset)
        '''
        TDILeaf._create_mod(self, key_fields, data_fields, code_str)

    # Generating mod_inc API dynamically
    def _create_mod_inc(self, key_fields, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} mod_flag=0, pipe=None, gress_dir=None):
    """ Incremental Add/Delete items in the fields that are array for the matched entry in {} table.

    Parameters:
    {}
    mod_flag: 0/1 value for Add/Delete Item in the field that is array type.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.mod_inc_entry(parsed_keys, parsed_data, mod_flag)
        '''
        TDILeaf._create_mod_inc(self, key_fields, data_fields, code_str)

    # Generating delete API dynamically
    def _create_del(self, key_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, handle=None):
    """Delete entry from {} table.

    Parameters:
    {}
    """
    parsed_keys, _ = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    self._c_tbl.del_entry(parsed_keys, entry_handle=handle)
        '''
        TDILeaf._create_del(self, key_fields, code_str)

    # Generating get API dynamically
    def _create_get(self, key_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} regex=False, return_ents=True, print_ents=True, table=False, from_hw=False, pipe=None, gress_dir=None, handle=None):
    """Get entry from {} table.
    If regex param is set to True, perform regex search on key fields.
    When regex is true, default for each field is to accept all.
    We use the python regex search() function:
    https://docs.python.org/3/library/re.html

    Parameters:
    {}
    regex: default=False
    from_hw: default=False
    """
    if regex or table:
        etd = TableEntryDumper(self._c_tbl)
        sts = self._c_tbl.dump(etd, from_hw=from_hw, print_ents=print_ents)
        if sts == 0 and print_ents:
            if regex:
                etd.print_table(regex_strs={})
            else:
                etd.print_table()
        if return_ents:
            return etd.dump_entry_objs(regex_strs={})
        return
    parsed_keys, _ = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return -1
    objs = self._c_tbl.get_entry(parsed_keys, print_entry=print_ents, from_hw=from_hw, entry_handle=handle)
    if return_ents:
        return objs
    return
        '''
        TDILeaf._create_get(self, key_fields, code_str)

    # Generating get_handle API dynamically
    def _create_get_handle(self, key_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} regex=False, return_ents=True, print_ents=True, table=False, from_hw=False, pipe=None, gress_dir=None):
    """Get entry handle for specific key from {} table.
    If regex param is set to True, perform regex search on key fields.
    When regex is true, default for each field is to accept all.
    We use the python regex search() function:
    https://docs.python.org/3/library/re.html

    Parameters:
    {}
    regex: default=False
    from_hw: default=False
    handle: default=None
    """
    if regex or table:
        etd = TableEntryDumper(self._c_tbl)
        sts = self._c_tbl.dump(etd, from_hw=from_hw, print_ents=print_ents)
        if sts == 0 and print_ents:
            if regex:
                etd.print_table(regex_strs={})
            else:
                etd.print_table()
        if return_ents:
            return etd.dump_entry_objs(regex_strs={})
        return
    parsed_keys, _ = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return -1
    objs = self._c_tbl.get_entry_handle(parsed_keys, print_entry=print_ents, from_hw=from_hw)
    if return_ents:
        return objs
    return
        '''
        TDILeaf._create_get_handle(self, key_fields, code_str)

    # Generating get_key API dynamically
    def _create_get_key(self):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, handle, return_ent=True, print_ent=True, from_hw=False, pipe=None, gress_dir=None):
    """Get entry key from {} table by entry handle.

    Parameters:
    handle: type=UINT64 size=32
    from_hw: default=False
    print_ents: default=True
    pipe: default=None (use global setting)
    """
    if handle < 0 or handle >= 2**(sizeof(c_uint32)*8):
        return -1
    objs = self._c_tbl.get_entry_key(entry_handle=handle, print_entry=print_ent, from_hw=from_hw)
    if return_ent:
        return objs
    return
        '''
        TDILeaf._create_get_key(self, code_str)

    # Generating get_default API dynamically
    def _create_get_default(self):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, return_ent=True, print_ent=True, from_hw=False, pipe=None, gress_dir=None):
    """Get default entry from {} table.

    Parameters:
    from_hw: default=False
    print_ents: default=True
    table: default=False
    pipe: default=None (use global setting)
    """
    objs = self._c_tbl.get_default_entry(print_entry=print_ent, from_hw=from_hw)
    if return_ent:
        return objs
    return
        '''
        TDILeaf._create_get_default(self, code_str)

    @target_check_and_set
    def info(self, pipe=None, return_info=False, print_info=True):
        TDILeaf.info(self, return_info, print_info)

    @target_check_and_set
    def clear(self, pipe=None, gress_dir=None, batch=True):
        TDILeaf.clear(self, batch=True)

    @target_check_and_set
    def dump(self, table=False, pipe=None, gress_dir=None, json=False, from_hw=False, return_ents=False, print_zero=True):
        TDILeaf.dump(self, table, json, from_hw, return_ents, print_zero)

class CIntfTdiRt(CIntfTdi):
    target_type_cls = TargetTypeRt
    leaf_type_cls = TdiRtLeaf

    def __init__(self, dev_id, table_cls, info_cls):
        driver_path = install_directory+'/lib/libdriver.so'
        super().__init__(dev_id, TdiRtTable, TdiInfo, driver_path)
        self._dev_tgt = self.TdiDevTgt(self._dev_id, 0, 0xff)
        
    class TdiDevTgt(Structure):
        # tdi_rt: target specific fields based on
        # 1. include/tdi/common/tdi_target.hpp   (core specific: TDI_TARGET_DEV_ID)
        # 2. include/tdi/arch/pna/pna_target.hpp (arch specific: PNA_TARGET_PIPE_ID, PNA_TARGET_DIRECTION)
        _fields_ = [("dev_id", c_int), ("pipe_id", c_uint), ("direction", c_uint)]
        def __str__(self):
            ret_val = ""
            for name, type_ in self._fields_:
                ret_val += name + ": " + str(getattr(self, name)) + "\n"
            return ret_val
    def _set_pipe(self, pipe=0xFFFF):
        self._dev_tgt = self.TdiDevTgt(self._dev_tgt.dev_id, pipe, self._dev_tgt.direction)

    def _set_direction(self, direction=0xFFFF):
        self._dev_tgt = self.TdiDevTgt(self._dev_tgt.dev_id, self._dev_tgt.pipe_id, direction)
    # this can be removed after changes from tdi repro changes with 4 parameters
    '''
    def _set_parser(self, parser=0xFF):
        self._dev_tgt = self.TdiDevTgt(self._dev_tgt.dev_id, self._dev_tgt.pipe_id, self._dev_tgt.direction, parser)
    '''
    def create_devTgt(self, dev_id, pipe_id=0, direction=0xff):
        return self.TdiDevTgt(self._dev_id, 0, 0xff)

    def print_target(self, target):
        dev_id = c_uint64(0);
        sts = self.get_driver().tdi_target_get_value(target, self.target_type_cls.target_type_map(target_type_str="dev_id"), byref(dev_id));
        pipe_id = c_uint64(0);
        sts = self.get_driver().tdi_target_get_value(target, self.target_type_cls.target_type_map(target_type_str="pipe_id"), byref(pipe_id));
        direction = c_uint64(0);
        sts = self.get_driver().tdi_target_get_value(target, self.target_type_cls.target_type_map(target_type_str="direction"), byref(direction));
        return "dev_id={} pipe={} direction={}".format(dev_id.value, pipe_id.value, direction.value)

class TdiRtCli(TdiCli):
    fixed_nodes=["port", "mirror", "fixed"]
    cIntf_cls = CIntfTdiRt
    leaf_cls = TdiRtLeaf

    def fill_dev_node(self, cintf, dev_node):
        dev_node.devcall = cintf._devcall
        dev_node.set_pipe = cintf._set_pipe
        dev_node.set_direction = cintf._set_direction
        dev_node.complete_operations = cintf._complete_operations
        dev_node.batch_begin = cintf._begin_batch
        dev_node.batch_flush = cintf._flush_batch
        dev_node.batch_end = cintf._end_batch
        dev_node.transaction_begin = cintf._begin_transaction
        dev_node.transaction_verify = cintf._verify_transaction
        dev_node.transaction_commit = cintf._commit_transaction
        dev_node.transaction_abort = cintf._abort_transaction
        dev_node.p4_programs_list = []

def start_tdi_rt(in_fd, out_fd, install_dir, dev_id_list, udf=None, interactive=False):
    global install_directory
    install_directory = install_dir
    tdi_rt_cli = TdiRtCli()
    return tdi_rt_cli.start_tdi(in_fd, out_fd, dev_id_list, udf, interactive)

def main():
    start_tdi_rt(0, 1, os.getcwd(), [0])

if __name__ == "__main__":
    main()
