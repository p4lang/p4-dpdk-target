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

def target_check_and_set(f):
    @functools.wraps(f)
    def target_wrapper(*args, **kw):
        cintf = args[0]._cintf
        pipe = None
        gress_dir = None
        prsr_id = None
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

class CIntfTdiRt(CIntfTdi):
    target_type_cls = TargetTypeRt
    def __init__(self, dev_id, table_cls, info_cls):
        driver_path = install_directory+'/lib/libdriver.so'
        super().__init__(dev_id, TdiRtTable, TdiInfo, driver_path)
        self._dev_tgt = self.TdiDevTgt(self._dev_id, 0, 0xff, 0xff)
        
    class TdiDevTgt(Structure):
        # tdi_rt: target specific fields based on
        # 1. include/tdi/common/tdi_target.hpp   (core specific: TDI_TARGET_DEV_ID)
        # 2. include/tdi/arch/pna/pna_target.hpp (arch specific: PNA_TARGET_PIPE_ID, PNA_TARGET_DIRECTION)
        _fields_ = [("dev_id", c_int), ("pipe_id", c_uint), ("direction", c_uint), ("prsr_id", c_ubyte)]
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
    def _set_parser(self, parser=0xFF):
        self._dev_tgt = self.TdiDevTgt(self._dev_tgt.dev_id, self._dev_tgt.pipe_id, self._dev_tgt.direction, parser)

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
    return tdi_rt_cli.start_tdi(in_fd, out_fd, install_dir, dev_id_list, udf, interactive)

def main():
    start_tdi_rt(0, 1, os.getcwd(), [0])

if __name__ == "__main__":
    main()
