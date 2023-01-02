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
import logging
import pdb

def target_check_and_set(f):
    @functools.wraps(f)
    def target_wrapper(*args, **kw):
        cintf = args[0]._cintf
        pipe = None
        gress_dir = None
        old_tgt = False

        for k,v in kw.items():
            if k == "pipe":
                pipe = v
            elif k == "gress_dir":
                gress_dir = v

        if pipe is not None or gress_dir is not None:
            _, old_pipe, old_gress_dir = cintf.get_target_vals(cintf._target)
            old_tgt = True
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
               cintf._set_pipe(pipe=old_pipe)
               cintf._set_direction(old_gress_dir)
            raise e
        if old_tgt:
            cintf._set_pipe(pipe=old_pipe)
            cintf._set_direction(old_gress_dir)
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
        name_lowercase_without_dollar=self.name.lower().replace("$","").replace("-","_")
        if self.table_type in ["PORT_CFG", "PORT_STAT", "PORT_HDL_INFO", "PORT_FRONT_PANEL_IDX_INFO", "PORT_STR_INFO"]:
            self.name = "port.{}".format(name_lowercase_without_dollar)
        if self.table_type in ["PRE_MGID", "PRE_NODE", "PRE_ECMP", "PRE_LAG", "PRE_PRUNE", "PRE_PORT", "MIRROR_CFG"]:
            self.name = name_lowercase_without_dollar
        if self.table_type in ["TM_PORT_GROUP_CFG", "TM_PORT_GROUP"]:
            self.name = name_lowercase_without_dollar
        if self.table_type in ["SNAPSHOT", "SNAPSHOT_LIVENESS"]:
            self.name = "{}".format(name_lowercase_without_dollar)
        if self.table_type in ["FIXED_FUNC"] or self.table_type in ["FIXED_FUNC_STATE"]:
            # temperated workaround for the table name is on node level with command,
            # in order to make it works with currently python CLI(The target independent code needs to add this feature).
            # Here is a list of dic for table name at node level and corresponding the leaf level table name.
            replace_name_dic = {"ipsec_offload": "ipsec_offload.notification"}
            if (name_lowercase_without_dollar in replace_name_dic.keys()):
                name_lowercase_without_dollar = replace_name_dic[name_lowercase_without_dollar]
                logging.debug("after translation: name_lowercase_without_dollar = "+name_lowercase_without_dollar)
            self.name = "fixed.{}".format(name_lowercase_without_dollar)

    def _wrap_ipsec_sadb_expire_notif_cb(self, callback):
        # callback_wrapper prototype should be the same as target specific

        # the callback_wrapper parameter should be the same prototype as
        # include/tdi_rt/c_frontend/tdi_rt_attributes.h 
        '''
        typedef void (*tdi_ipsec_sadb_expire_cb)(uint32_t dev_id,
                                         uint32_t ipsec_sa_api,
                                         bool soft_lifetime_expire,
                                         uint8_t ipsec_sa_protocol,
                                         char *ipsec_sa_dest_address,
                                         bool ipv4,
                                         void *cookie);
        '''
        # The callback_wrapper should be the same function prototype as in include/tdi_rt/c_frontend/tdi_rt_attributes.h file
        def callback_wrapper(dev_id, ipsec_sa_api, soft_lifetime_expire, ipsec_sa_protocol, ipsec_sa_dest_address, ipv4, cookie):
            callback(dev_id, ipsec_sa_api, soft_lifetime_expire, ipsec_sa_protocol, ipsec_sa_dest_address, ipv4, cookie)
            return 0
        return self._cintf.ipsec_sadb_expire_notif_cb_type(callback_wrapper)

    #expire notif_cb
    def set_ipsec_sadb_expire_notif_cb(self, callback, enable, cookie):
        attr_hdl = self._cintf.handle_type()
        #sts = self._cintf.get_driver().tdi_table_ipsec_expire_notif_attributes_allocate(self._handle, byref(attr_hdl))
        logging.debug("ipsec_sadb_expire_notif_cb type enum = {}".format(self.attributes_type_cls.attributes_rev_dict["ipsec_sadb_expire_notif_cb"]))
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["ipsec_sadb_expire_notif_cb"], byref(attr_hdl))
        if sts != 0:
            print("ipsec_sadb_expire_notif_cb tdi_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            return -1
        # registered callback function
        self.ipsec_sadb_expire_notif_callback = self._wrap_ipsec_sadb_expire_notif_cb(callback)
        '''
        sts = self._cintf.get_driver().tdi_attributes_ipsec_sadb_expire_notify_set(attr_hdl,
                                                                                   c_bool(bool(True)),
                                                                                   self.ipsec_sadb_expire_notif_callback,
                                                                                   c_void_p(0))
        '''
        # tdi_attributes_set_value need based on the tdi target defs.h
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                0,
                c_bool(bool(enable)))
        if not sts == 0:
            print("ipsec_sadb_expire_set_value for enable failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                2,
                self.ipsec_sadb_expire_notif_callback)
        if not sts == 0:
            print("ipsec_sadb_expire_set_value for ipsec_sadb_expire_notif_callback failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                3,
                c_void_p(cookie))
        if not sts == 0:
            print("ipsec_sadb_expire_set_value for cookie failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def get_ipsec_sadb_expire_notif_cb(self):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["ipsec_sadb_expire_notif_cb"], byref(attr_hdl))
        if not sts == 0:
            print("ipsec_sadb_expire_notif_cb tdi_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            return -1
        sts = self._attr_get(attr_hdl)
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            return -1
        enable = c_bool()
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                0,
                byref(enable))
        if not sts == 0:
            print("tdi_attributes_get_value for enable failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        cookie=c_uint64()
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                3,
                byref(cookie))
        if not sts == 0:
            print("ipsec_sadb_expire_set_value for cookie failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_deallocate(attr_hdl)
        get_value = {"enable": enable.value, "cookie": cookie.value}
        return {"enable": enable.value, "cookie": cookie.value}
