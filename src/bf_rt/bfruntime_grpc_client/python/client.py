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

#@package docstring
#Documentation for this module.
#More details.

import time
import grpc
import bfrt_grpc.bfruntime_pb2_grpc as bfruntime_pb2_grpc
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
from . import info_parse as info_parse

import socket
import struct
try:
    import queue as q
except ImportError as e:
    import Queue as q
import logging
import threading
import json
import sys
import random

import google.rpc.status_pb2 as status_pb2
import google.rpc.code_pb2 as code_pb2

from collections import OrderedDict
from functools import total_ordering
import codecs
import binascii

logger = logging.getLogger('bfruntime_grpc_client')
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)

def is_python2():
    return sys.version_info < (3, 0)

def to_bytes(n, length):
    """ Convert integers to bytes. """
    h = '%x' % n
    mask = 2**(length*8) - 1
    # In case of negative numbers, mask must be applied to point out the size
    if (n < 0 and n.bit_length() <= mask.bit_length()):
        h = '%x' % (n & mask)
    s = ('0'*(len(h) % 2) + h).zfill(length*2)
    return bytearray(codecs.decode(s, "hex"))

def bytes_to_int(byte_s):
    return int(binascii.hexlify(bytes(byte_s)), 16)

def ipv4_to_bytes(addr):
    """ Convert Ipv4 address to a bytearray. """
    if (isinstance(addr, bytes)):
        addr = addr.decode()
    val = [int(v) for v in addr.encode("utf-8").decode().split('.')]
    return bytearray(val)

def ipv6_to_bytes(addr):
    """ Convert Ipv6 address to a bytes. """
    if (isinstance(addr, bytes)):
        addr = addr.decode()
    return bytearray(socket.inet_pton(socket.AF_INET6, addr))

def mac_to_bytes(addr):
    """ Covert Mac address to a bytes. """
    if (isinstance(addr, bytes)):
        addr = addr.decode()
    val = [int(v, 16) for v in addr.encode("utf-8").decode().split(':')]
    return bytearray(val)

def bytes_to_ipv4(byte_s):
    int2ip = lambda addr: socket.inet_ntoa(struct.pack("!I", addr))
    return int2ip(int(binascii.hexlify(bytes(byte_s)), 16))

def bytes_to_ipv6(byte_s):
    int2ip6 = lambda addr: socket.inet_ntop(socket.AF_INET6, addr)
    return int2ip6(bytes(byte_s))

def bytes_to_mac(byte_s):
    ret=""
    if (not isinstance(byte_s, bytearray)):
        byte_s = bytearray(byte_s)
    for b in byte_s:
        if b < 16:
            ret += "0%x:" % b
        else:
            ret += "%x:" % b
    return ret[:-1]

def _convert_to_bytearray(value, f_name, f_size, f_annotations):
    ret_val = None
    if isinstance(value, int) or (is_python2() and isinstance(value, long)):
        # If int, then change to bytearray
        ret_val = to_bytes(value, f_size) # to_bytes adds extra padding if required for int
    elif isinstance(value, bytearray):
        # If bytearray, then just pad and verify size
        if len(value) < f_size:  # Pad if passed in size is less than actual size
            len_to_pad = f_size - len(value)
            ret_val = bytearray(len_to_pad) + value
        elif len(value) == f_size:
            ret_val = value # size passed in is equal to actual size
        else:
            ValueError("Field %s Passed in size %d is > actual field size %d"\
                    %(f_name, len(value), f_size))
    elif isinstance(value, str) or (is_python2() and isinstance(value, unicode)):
        if is_python2() and isinstance(value, unicode):
            value = value.encode('ascii')
        # Check if annotations are set for mac or ip
        if "$client_annotation.ipv4" in f_annotations:
            ret_val = ipv4_to_bytes(value)
        elif "$client_annotation.ipv6" in f_annotations:
            ret_val = ipv6_to_bytes(value)
        elif "$client_annotation.mac" in f_annotations:
            ret_val = mac_to_bytes(value)
        else:
            raise TypeError("Cannot set as a %s for field %s. Please set"
                    "correct annotations to use this"\
                    % (type(value), f_name))
    else:
        ret_val = value

    return ret_val

def _convert_to_presentation(value, f_name, f_size, f_annotations):
    ret_val = None
    if isinstance(value, bytearray):
        # if value is a bytearray, then check annotations and convert
        # to the correct thing. If "bytes" annotation is set, then don't
        # do anything
        # Convert to int if no annotations valid
        if "$client_annotation.ipv4" in f_annotations:
            ret_val = bytes_to_ipv4(value)
        elif "$client_annotation.ipv6" in f_annotations:
            ret_val = bytes_to_ipv6(value)
        elif "$client_annotation.mac" in f_annotations:
            ret_val = bytes_to_mac(value)
        elif "$client_annotation.bytes" in f_annotations:
            ret_val = value
        else:
            ret_val = bytes_to_int(value)
    elif isinstance(value, int) or (is_python2() and isinstance(value, long)):
        ret_val = value
    elif is_python2() and isinstance(value, unicode):
        ret_val = value.encode('ascii')
    elif value is None:
        ret_val = None
    else:
        raise TypeError("Invalid type %s for field %s encountered!!"\
                %(type(value), f_name))
    return ret_val


def print_grpc_error(grpc_error):
    """@brief This function can be used to parse an error returned by Write/Read API
        calls. The Write/Read API calls return a status with top level UNKNOWN status
        when even a single entry read/write entry in the batch fails. All the details per
        entry are embedded in the status msg which can be parsed by this function and
        printed
        @return Dictionary of (index, error-msg) of the entries which failed
    """
    def _parse_grpc_error_binary_details(grpc_error):
        if grpc_error.code() != grpc.StatusCode.UNKNOWN:
            return None

        error = None
        # The gRPC Python package does not have a convenient way to access the
        # binary details for the error: they are treated as trailing metadata.
        for meta in grpc_error.trailing_metadata():
            if meta[0] == "grpc-status-details-bin":
                error = status_pb2.Status()
                error.ParseFromString(meta[1])
                break
        if error is None:  # no binary details field
            return None
        if len(error.details) == 0:
            # binary details field has empty Any details repeated field
            return None

        indexed_p4_errors = []
        for idx, one_error_any in enumerate(error.details):
            p4_error = bfruntime_pb2.Error()
            if not one_error_any.Unpack(p4_error):
                return None
            if p4_error.canonical_code == code_pb2.OK:
                continue
            indexed_p4_errors += [(idx, p4_error)]
        return indexed_p4_errors

    status_code = grpc_error.code()
    logger.error("gRPC Error %s %s",
            grpc_error.details(),
            status_code.name)

    if status_code != grpc.StatusCode.UNKNOWN:
        return
    bfrt_errors = _parse_grpc_error_binary_details(grpc_error)
    if bfrt_errors is None:
        return
    logger.error("Errors in batch:")
    for idx, bfrt_error in bfrt_errors:
        code_name = code_pb2._CODE.values_by_number[
            bfrt_error.canonical_code].name
        logger.error("\t* At index %d %s %s\n",
                idx, code_name, bfrt_error.message)
    return bfrt_errors

class BfruntimeRpcException(Exception):
    """@brief The main parent bfruntime exception class.
    All clients can catch just this exception and error out
    by printing the exception for ease.
    If required, then the member grpc.RpcError object can be
    retrieved from grpc_error_get() function.
    If there can be multiple errors in the exception, for example
    one error each for batch operations, then the sub-error list
    can be retrieved by sub_errors(). This sub-error list will contain
    errors as tuples of (idx, p4_error), where p4_error is
    an instance of bfruntime_pb2.Error() protobuf message.
    If there is just one single main error, then this list will
    be empty.
    """
    def __init__(self, grpc_error):
        super(BfruntimeRpcException, self).__init__()
        self.grpc_error = grpc_error
        self.errors = []

    def __str__(self):
        return super(BfruntimeRpcException, self).__str__()

    def grpc_error_get(self):
        return self.grpc_error
    def sub_errors_get(self):
        return self.errors

class BfruntimeSubscribeRpcException(BfruntimeRpcException):
    def __init__(self, grpc_error):
        assert(grpc_error.code() != grpc.StatusCode.UNKNOWN)
        super(BfruntimeSubscribeRpcException, self).__init__(grpc_error)

    def __str__(self):
        message = "Error: "
        message += str(self.grpc_error.code())
        message += " : " + self.grpc_error.debug_error_string()
        return message

class BfruntimeForwardingRpcException(BfruntimeRpcException):
    def __init__(self, grpc_error):
        assert(grpc_error.code() != grpc.StatusCode.UNKNOWN)
        super(BfruntimeForwardingRpcException, self).__init__(grpc_error)

    def __str__(self):
        message = "Error: "
        message += str(self.grpc_error.code())
        message += " : " + self.grpc_error.debug_error_string()
        return message

class BfruntimeReadWriteRpcException(BfruntimeRpcException):
    class _BfruntimeErrorIterator:
        """@brief  Used to iterate over the p4.Error messages in a gRPC error Status object
        """
        def __init__(self, grpc_error):
            assert(grpc_error.code() == grpc.StatusCode.UNKNOWN)
            if grpc_error.code() != grpc.StatusCode.UNKNOWN:
                return None
            self.grpc_error = grpc_error

            error = None
            # The gRPC Python package does not have a convenient way to access the
            # binary details for the error: they are treated as trailing metadata.
            for meta in self.grpc_error.trailing_metadata():
                if meta[0] == "grpc-status-details-bin":
                    error = status_pb2.Status()
                    error.ParseFromString(meta[1])
                    break
            if error is None:
                raise RuntimeError("No binary details field")

            if len(error.details) == 0:
                raise RuntimeError(
                    "Binary details field has empty Any details repeated field")
            self.errors = error.details
            self.idx = 0

        def __iter__(self):
            return self

        def next(self):
            return self.__next__()

        def __next__(self):
            while self.idx < len(self.errors):
                p4_error = bfruntime_pb2.Error()
                one_error_any = self.errors[self.idx]
                if not one_error_any.Unpack(p4_error):
                    raise RuntimeError(
                        "Cannot convert Any message to p4.Error")
                v = self.idx, p4_error
                self.idx += 1
                if p4_error.canonical_code == code_pb2.OK:
                    continue
                return v
            raise StopIteration

    def __init__(self, grpc_error):
        super(BfruntimeReadWriteRpcException, self).__init__(grpc_error)
        # The only case where a Read/Write RPC returns an error which
        # can be looked inside is an UNKNOWN error. Just store other
        # error in self.grpc_error in other cases
        if grpc_error.code() != grpc.StatusCode.UNKNOWN:
            return
        try:
            error_iterator = self._BfruntimeErrorIterator(grpc_error)
            for error_tuple in error_iterator:
                self.errors.append(error_tuple)
        except RuntimeError as e:
            raise  # just propagate exception for now

    def __str__(self):
        message = "Error(s):\n"
        for idx, p4_error in self.errors:
            code_name = code_pb2._CODE.values_by_number[
                p4_error.canonical_code].name
            message += "\t* At index {}: {}, '{}'\n".format(
                idx, code_name, p4_error.message)
        return message

def _cpy_target(req, target_src):
        req.target.device_id = target_src.device_id_
        req.target.pipe_id = target_src.pipe_id_
        req.target.direction = target_src.direction_
        req.target.prsr_id = target_src.prsr_id_

class ClientInterface:
    """@brief Class to provide an interface to connect to a remote switch and
    to be used during setUp. It provides methods to subscribe,
    GetForwardingPipelineConfig, SetForwardingPipelineConfig.
    """
    class _ReaderWriterInterface:
        """@brief (Internal). It wraps the read/write implementation for a specific client.
        """
        def __init__(self, stub, client_id):
            """@brief Internal Initialize the Interface
                @param stub The internal stub object which will be used to write/read to
                @param client_id Client-ID
            """
            self.client_id = client_id
            self.stub = stub

        def _write(self, req, metadata=None):
            """@brief Internal Send Write req to the client
                @param Request to be sent
                @param metadata : optional metadata to send with write request
            """
            req.client_id = self.client_id
            try:
                self.stub.Write(req, metadata=metadata)
            except grpc.RpcError as e:
                raise BfruntimeReadWriteRpcException(e)

        def _read(self, req, metadata=None):
            """@brief Internal Send Read req to the client
                @param Request to be sent
                @param metadata : optional metadata to send with write request
            """
            req.client_id = self.client_id
            try:
                return self.stub.Read(req, metadata=metadata)
            except grpc.RpcError as e:
                raise BfruntimeReadWriteRpcException(e)

    def __init__(self, grpc_addr, client_id, device_id,
            notifications=None, timeout=1, num_tries=5, perform_subscribe=True):
        """@brief The ClientInterface object requires both of the endpoints' info like
            remote-switch address and self's client_id, device_id . Init will lay the
            groundwork to connect to a remote-switch like creating an insecure_channel
            and setting up queues and threads for incoming streams. It also sends a
            subscribe request and retries at most num_tries times

            @param grpc_addr The address of the grpc server and the port number separated
            by a colon
            @param client_id The client ID of the client which it wishes to have
            @param device_id The deivce ID to which it wants to connect to on the
            remote switch
            @param notifications A Notifications object.
            @param num_tries Max number of tries on subscribing before erroring out.
            Default = 5
            @param perform_subscribe If the client wants to subscribe for
            notifications Default = True
            @param timeout Max timeout to wait for for subscribe message to succeed
            @exception RuntimeError If failed to subscribe within num_tries
        """
        # If subscribe not requested, there should be no notifications
        if not perform_subscribe and notifications:
            raise RuntimeError("Notifications should not be provided if subscribe not \
                requested")

        self.is_independent = not perform_subscribe
        self.client_id = client_id
        self.device_id = device_id
        self.stream = None
        gigabyte = 1024 ** 3
        self.channel = grpc.insecure_channel(grpc_addr, options=[
            ('grpc.max_send_message_length', gigabyte), (
                'grpc.max_receive_message_length', gigabyte),
                ('grpc.max_metadata_size', gigabyte)])

        self.stub = bfruntime_pb2_grpc.BfRuntimeStub(self.channel)
        self.reader_writer_interface = self._ReaderWriterInterface(self.stub, self.client_id)
        self.stream_out_q = q.Queue()
        self.stream_in_q = q.Queue()
        self.exception_q = q.Queue()
        # Subscribe
        if perform_subscribe:
            self.set_up_stream()
            success = self.subscribe(notifications=notifications, timeout=timeout,
                num_tries=num_tries)
            if not success:
                raise RuntimeError("Failed to subscribe to server at %s", grpc_addr)

    def __del__(self):
        """@brief Deletes the stream explicitly while destroying this object
        """
        if not self.is_independent:
            self.tear_down_stream()

    def bind_pipeline_config(self, p4_name):
        """@brief Bind to a Program on the device
            @param p4_name Name of the P4 program this client wishes to bind to
            @exception ValueError If empty p4-name is passed
            @exception RpcError On gRPC error on sending SetForwardingPipelineConfig
            request
        """
        if not p4_name or p4_name == "":
            logger.error("Cannot bind with empty p4_name")
            raise ValueError("Cannot bind with empty p4_name")
        req = bfruntime_pb2.SetForwardingPipelineConfigRequest()
        req.client_id = self.client_id;
        req.action = bfruntime_pb2.SetForwardingPipelineConfigRequest.BIND;
        config = req.config.add()
        config.p4_name = p4_name
        logger.info("Binding with p4_name " + p4_name)
        try:
            self.stub.SetForwardingPipelineConfig(req)
        except grpc.RpcError as e:
            raise BfruntimeForwardingRpcException(e)
        logger.info("Binding with p4_name %s successful!!", p4_name)

    def bfrt_info_get(self, p4_name=None):
        """@brief Get bf-rt info json from the client as part of GetForwardingPipelineConfig
            and then parse it locally to create a _BfRtInfo object.
            @param p4_name Name of the P4 program this client wishes to get the bf-rt json for.
            If no p4_name is given, then this function returns a _BfRtInfo object made from the
            device's first current P4 program.
            @return _BfRtInfo object.
            @exception RuntimeError On not receiving bf-rt-info.json when expecting it
        """
        # send a request
        req = bfruntime_pb2.GetForwardingPipelineConfigRequest()
        req.device_id = self.device_id
        req.client_id = self.client_id
        try:
            msg = self.stub.GetForwardingPipelineConfig(req)
        except grpc.RpcError as e:
            raise BfruntimeForwardingRpcException(e)

        # get the reply
        if msg is None:
            raise RuntimeError("BF_RT_INFO not received")
        else:
            for config in msg.config:
                logger.info("Received %s on GetForwarding on client %d, device %d", config.p4_name, req.client_id, req.device_id)
            if not p4_name:
                return _BfRtInfo(msg.config[0].p4_name, msg.config[0].bfruntime_info, msg.non_p4_config.bfruntime_info, self.reader_writer_interface)
            for config in msg.config:
                if (p4_name == config.p4_name):
                    return _BfRtInfo(config.p4_name, config.bfruntime_info, msg.non_p4_config.bfruntime_info, self.reader_writer_interface)

            raise RuntimeError("BF_RT_INFO not received")

    def get_packet_in(self, timeout=1):
        """@brief Not supported right now
        """
        pass

    def subscribe(self, notifications = None, timeout=1, num_tries=5):
        """@brief Wait for a success response from the server until timeout for each try.
            @param timeout Timeout to wait for in seconds for each retry. Default = 1
            @param notifications A Notifications object. If sent None, then the default
            Notifications object is created and passed down.
            @param num_tries Number of retries. Default = 5
            @return bool subscribe was successful or not
        """
        def _send_subscribe(self, notifications):
            """@brief Send a subscribe message to the remote server
                @param notifications Notifications object
            """
            req = bfruntime_pb2.StreamMessageRequest()
            req.client_id = self.client_id
            req.subscribe.device_id = self.device_id

            req.subscribe.notifications.enable_learn_notifications =\
                notifications.enable_learn_notifications
            req.subscribe.notifications.enable_idletimeout_notifications =\
                notifications.enable_idletimeout_notifications
            req.subscribe.notifications.enable_port_status_change_notifications =\
                notifications.enable_port_status_change_notifications

            self.stream_out_q.put(req)

        def _get_response(self, timeout):
            msg = self._get_stream_message("subscribe", timeout)
            if msg is None:
                logger.info("Subscribe timeout exceeded %ds", timeout)
                return False
            else:
                logger.info("Subscribe response received %d", msg.subscribe.status.code)
                if (msg.subscribe.status.code != code_pb2.OK):
                    logger.info("Subscribe failed")
                    return False
            return True

        if notifications is None:
            notifications = Notifications()
        cur_tries = 0
        success = False

        while(cur_tries < num_tries and not success):
            _send_subscribe(self, notifications)
            logger.info("Subscribe attempt #%d", cur_tries+1)
            # Wait for 5 seconds max for each attempt
            success = _get_response(self, timeout)
            cur_tries += 1
        return success

    def is_set_fwd_action_done(self, value_to_check_for, timeout=5, num_tries=5):
        """@brief Wait for a SetForwardingPipelineConfigResponse message. This function
            is internally also being used by sendSetForwardingPipelineConfigRequest as
            well. This can be used separately if a client wants to check for
            a SetForwarding response on its own stream.
            @param value_to_check_for  the response type to wait for. This is of the proto
            enum type.
            @param timeout Timeout to wait for in seconds for each retry. default = 1
            @param num_tries Max number of tries. Default = 5
            @return bool the expected msg was received or not
        """
        cur_tries = 0
        success = False
        while(cur_tries < num_tries and not success):
            msg = self._get_stream_message("set_forwarding_pipeline_config_response", timeout)
            if msg is None:
                logger.info("commit notification expectation exceeded %ds", timeout)
                success = False
            else:
                if (msg.set_forwarding_pipeline_config_response.
                        set_forwarding_pipeline_config_response_type == value_to_check_for ==
                        bfruntime_pb2.SetForwardingPipelineConfigResponseType.Value("WARM_INIT_STARTED")):
                    logger.info("WARM_INIT_STARTED received")
                    success = True
                elif (msg.set_forwarding_pipeline_config_response.
                        set_forwarding_pipeline_config_response_type == value_to_check_for ==
                        bfruntime_pb2.SetForwardingPipelineConfigResponseType.Value("WARM_INIT_FINISHED")):
                    logger.info("WARM_INIT_FINISHED received")
                    success = True
            cur_tries += 1
        return success

    def send_set_forwarding_pipeline_config_request(self,
            action=bfruntime_pb2.SetForwardingPipelineConfigRequest.VERIFY,
            base_path="",
            forwarding_config_list=[],
            dev_init_mode=bfruntime_pb2.SetForwardingPipelineConfigRequest.FAST_RECONFIG,
            timeout=5,
            num_tries=5):
        """@brief Send a SetForwardingPipelineConfigRequest msg and then
            wait for a SetForwardingPipelineConfigResponse message
            @param action (string) the response type to wait for.
            @param timeout Timeout to wait for in seconds for each retry. default = 1
            @param number of retries. Default = 5
            @return bool the expected msg was received or not
        """
        def _send_request(action, base_path, forwarding_config_list, dev_init_mode):
            req = bfruntime_pb2.SetForwardingPipelineConfigRequest()
            req.client_id = self.client_id
            req.device_id = self.device_id
            req.base_path = base_path
            req.action = action
            req.dev_init_mode = dev_init_mode
            for fwd_config in forwarding_config_list:
                self._add_config_to_set_forward_request(req,
                    fwd_config.p4_name,
                    fwd_config.bfruntime_info_file,
                    fwd_config.profile_info_list)
            try:
                self.stub.SetForwardingPipelineConfig(req)
            except grpc.RpcError as e:
                raise BfruntimeForwardingRpcException(e)

        def _action_to_response_type(action):
            if action == bfruntime_pb2.SetForwardingPipelineConfigRequest\
                    .VERIFY_AND_WARM_INIT_BEGIN:
                return "WARM_INIT_STARTED"
            elif action == bfruntime_pb2.SetForwardingPipelineConfigRequest\
                    .VERIFY_AND_WARM_INIT_BEGIN_AND_END:
                return "WARM_INIT_FINISHED"
            elif action == bfruntime_pb2.SetForwardingPipelineConfigRequest\
                    .WARM_INIT_END:
                return "WARM_INIT_FINISHED"
            else:
                return ""

        _send_request(action, base_path, forwarding_config_list, dev_init_mode)
        response_type = _action_to_response_type(action)
        if response_type == "":
            # Not expecting a response for an action having no response type
            return True

        # Skip response if client is independent
        if self.is_independent:
            return True

        success = self.is_set_fwd_action_done(\
                bfruntime_pb2.SetForwardingPipelineConfigResponseType.Value(response_type),
                timeout, num_tries)
        return success

    def digest_get(self, timeout=1):
        """@brief Get a digest entry from the StreamChannel.
           @param timeout Timeout to wait for in seconds. default = 1
           @return Digest. It returns None if timeout exceeds
           @exception RuntimeError Upon not receiving a msg when expecting it
        """
        msg = self._get_stream_message("digest", timeout)
        if msg is None:
            raise RuntimeError("Digest list not received.")
        else:
            return msg.digest
        return None

    def digest_get_iterator(self, timeout=1):
        """@brief Get an iterator to digest entries from the StreamChannel.
           @param timeout  Timeout to wait for in seconds for each iteration.
                           default = 1

           @return Digest. It returns None if timeout exceeds
           @exception RuntimeError Upon not receiving a msg when expecting it
        """
        msg = self._get_stream_message("digest", timeout)
        while msg is not None:
            yield msg.digest
            msg = self._get_stream_message("digest", timeout)

    def idletime_notification_get(self, timeout=1):
        """@brief Get an idletimeout notification from the StreamChannel
           @param timeout Timeout to wait for in seconds. default = 1
           @return idletimeout notification entry. Returns None if timeout exceeds
           @exception RuntimeError Upon not receiving a msg when expecting it
        """
        msg = self._get_stream_message("idle_timeout_notification", timeout)
        if msg is None:
            raise RuntimeError("Idletime notification not received")
        else:
            return msg.idle_timeout_notification
        return None

    def portstatus_notification_get(self, timeout=1):
        """@brief Get a portstatus change notification from the StreamChannel
           @param timeout Timeout to wait for in seconds. default = 1
           @return portstatus change notification entry
           @exception RuntimeError Upon not receiving a msg when expecting it
        """
        msg = self._get_stream_message("port_status_change_notification", timeout)
        if msg is None:
            raise RuntimeError("port_status_change_notification not received.")
        else:
            return msg.port_status_change_notification
        return None


    def tear_down_stream(self):
        """@brief Tear down the thread listening for incoming msgs
        """
        # If stream is torn down, client is now indepedent
        self.stream_out_q.put(None)
        self.stream_recv_thread.join()
        self.is_independent = True

    def _get_stream_message(self, type_, timeout=1):
        """@brief Get a msg of a certain type from the in_queue
        """
        start = time.time()
        try:
            while True:
                remaining = timeout - (time.time() - start)
                if remaining < 0:
                    break
                msg = self.stream_in_q.get(timeout=remaining)
                if not msg.HasField(type_):
                    # Put the msg back in for someone else to read
                    # TODO make separate queues for each msg type
                    self.stream_in_q.put(msg)
                    continue
                return msg
        except:  # timeout expired
            pass
        return None

    def get_and_set_pipeline_config(self):
        req = bfruntime_pb2.GetForwardingPipelineConfigRequest()
        req.device_id = self.device_id
        req.client_id = self.client_id
        msg = self.stub.GetForwardingPipelineConfig(req)

        set_req = bfruntime_pb2.SetForwardingPipelineConfigRequest()
        set_req.client_id = self.client_id
        set_req.device_id = self.device_id
        set_req.base_path = ""
        set_req.action = bfruntime_pb2.SetForwardingPipelineConfigRequest\
                .VERIFY_AND_WARM_INIT_BEGIN_AND_END
        for get_config in msg.config:
            config = set_req.config.add()
            config.CopyFrom(get_config)
        self.stub.SetForwardingPipelineConfig(set_req)
        # get response
        success = self.is_set_fwd_action_done(\
                bfruntime_pb2.SetForwardingPipelineConfigResponseType.Value("WARM_INIT_FINISHED"),
                5, 5)
        return success

    def clear_all_tables(self):
        """@brief Clear all the tables of the device (P4 and non-P4 )
        this interface is connected to. Right now, we get and set fwd pipeline
        in order to clear. This beta version of this API is a device_reset
        right now and so port and TM counters will also be reset. Hence heed
        caution while using this especially on hw.
        Future work - manually iterate through all the tables and call
        clear() if available. Instead of clearing all the tables on the device,
        this API will clear all tables of the P4 program this interface is
        connected to. It will also have advanced options to clear either all
        P4 tables of this program, all non-P4 tables or all tables.
        """
        self.get_and_set_pipeline_config()

    def _add_config_to_set_forward_request(self, req, p4_name, bfruntime_info,
            input_profiles):
        def read_file(file_name):
            data = ""
            with open(file_name, 'rb') as myfile:
                data=myfile.read()
            return data
        config = req.config.add()
        config.p4_name = p4_name
        config.bfruntime_info = read_file(bfruntime_info)
        for input_profile in input_profiles:
            profile = config.profiles.add()
            profile.profile_name = input_profile.profile_name
            profile.context = read_file(input_profile.context_json_file)
            profile.binary = read_file(input_profile.binary_file)
            profile.pipe_scope.extend(input_profile.pipe_scope)

    def set_up_stream(self):
        def _stream_iterator():
            while True:
                p = self.stream_out_q.get()
                if p is None:
                    break
                yield p

        def _stream_recv(stream):
            try:
                for p in stream:
                    self.stream_in_q.put(p)
            except grpc.RpcError as e:
                self.exception_q.put(BfruntimeSubscribeRpcException(e))

        # If stream is set up, client is no longer independent
        self.is_independent = False
        self.stream = self.stub.StreamChannel(_stream_iterator())
        self.stream_recv_thread = threading.Thread(
            target=_stream_recv, args=(self.stream,))
        self.stream_recv_thread.daemon = True
        self.stream_recv_thread.start()

class Target:
    """@brief Class to create a Target. Most table APIs take in target as a parameter.
    """
    def __init__(self, device_id=0, pipe_id=0xffff, direction=0xff, prsr_id=0xff):
        """@brief Create the Target object
           @param device_id The device ID. Default = 0
           @param pipe_id The pipe ID. If the architecture supports 4 pipes, then this value can be 0,1,2,3.
           If the table to be programmed has certain scopes, then this value needs to be of the lowest pipe
           in the intended scope. Default = 0xffff which is all scopes
           @param direction Ingress/Egress
           @param prsr_id Parser ID

        """
        self.device_id_ = device_id
        self.pipe_id_ = pipe_id
        self.direction_ = direction
        self.prsr_id_ = prsr_id

    def __eq__(self, other):
        return ((self.device_id_, self.pipe_id_, self.direction_, self.prsr_id_) ==
            (other.device_id_, other.pipe_id_, other.direction_, other.prsr_id_))
    def __str__(self):
        msg = "dev_id: " + str(self.device_id_)
        msg += "\npipe_id: " + str(self.pipe_id_)
        msg += "\ndirection: " + str(self.direction_)
        msg += "\nparser_id: " + str(self.prsr_id_)
        return msg

class ProfileInfo:
    """@brief Class to create a helpful structure to keep per Pipeline
        profile data.
        This class takes in file names as inputs and not the actual
        file_contents.
    """
    def __init__(self, profile_name,
            context_json_file="", binary_file="", pipe_scope=[]):
        """@brief
        @param profile_name The profile name
        @param context_json_file File path of context.json
        @param binary_file File path of the .bin file
        @pipe_scope A list of the pipe_ids where this profile needs
        to be put. For example, [0,3] for pipes 0 and 3 to be in the same
        scope
        """
        self.profile_name = profile_name
        self.context_json_file= context_json_file
        self.binary_file = binary_file
        self.pipe_scope = pipe_scope

class ForwardingConfig:
    """@brief Class to create a Forwarding Config object for a P4 program.
        This class takes in file names as inputs and not the actual file_contents.
        We send the actual file contents over gRPC channel though.
        SetForwardingConfig requires a list of these objects since there can be
        multiple programs on a device.
    """

    def __init__(self, p4_name, bfruntime_info_file="", profile_info_list=[]):
        """@brief Create Forwarding Config object
        @param p4_name P4 program name
        @param bfruntime_info File path of bf-rt.json file
        @param profile_info_list list of ProfileInfo objects
        """
        self.p4_name = p4_name
        self.bfruntime_info_file = bfruntime_info_file
        self.profile_info_list = profile_info_list

class Notifications:
    """@brief Class to contain notification flags to be sent for subscribe.
        Unlike usual conventions where bool parameters are kept default False,
        here they are true. This is to avoid unnecessary confusions.
    """
    def __init__(self, enable_learn=True, enable_idletimeout=True, enable_port_status_change=True):
        """@brief Create a Notifications Object.
        @param enable_learn_notifications If learn notifications are required.
        @param enable_idletimeout_notifications If learn notifications are required.
        @param enable_port_status_change_notifications If learn notifications are required.
        """
        self.enable_learn_notifications = enable_learn
        self.enable_idletimeout_notifications = enable_idletimeout
        self.enable_port_status_change_notifications = enable_port_status_change

@total_ordering
class DataTuple:
    """@brief Class to create a DataTuple. Apart from the name, only one of them can be set at
        a time. Since this class has been kept separate from the _Table class, it doesn't perform
        sanity checks along the lines of being in parity with whether the types match with that
        of the table or not
    """
    def __init__(self, name, val=None, float_val=None,
            str_val=None, int_arr_val=None, bool_arr_val=None, bool_val=None,
            container_arr_val=None, str_arr_val=None):
        """@brief Create a DataTuple object. A list of these objects are taken in as parameters
            to the constructor of Data object. Size and type checks are not strict while creating
            an object of this class. Those checks are done while make_data is called on the _Table

            @param name Name of the field
            @param val bytearray/int/string value. This value is expected when
            type is either uint64, uint32, uin16, uint8 or bytes. It should be either
            bytearray or an int. A string can be accepted in some cases of custom
            annotations like IP and Mac addresses.
            @param float_val Float value. This value is expected when the type of the
            field is of float
            @param str_val String value. This value is expected when the type of the
            field is String
            @param bool_val Bool value. This value is expected when the type of the
            field is bool
            @param int_arr_val Integer list. This value is expected when the type of
            field is a repeated int
            @param bool_arr_val Bool list. This value is expected when the type of
            field is a repeated bool
            @param container_arr_val List of  field_dicts(field_name to DataTuple).
            This is used internally for container get purposes. NOT SUPPORTED for external use.
            @param str_arr_val String list. This value is expected when the type of
            field is a repeated string
            @exception TypeError is raised when an unexpected type is encountered for
            a certain parameter
        """

        if val is not None:
            if not isinstance(val, bytearray) and not isinstance(val, int)\
                    and not isinstance(val, str)\
                    and not isinstance(val, int)\
                    and not (is_python2() and isinstance(val, basestring))\
                    and not (is_python2() and isinstance(val, long)):
                raise TypeError("Please pass an int/long or bytearray. you passed %s"\
                        %(type(val)))
        elif float_val is not None:
            if not isinstance(float_val, float):
                raise TypeError("Please pass a float for float_val")
        elif str_val is not None:
            if not isinstance(str_val, str)\
                    or (is_python2() and not isinstance(str_val, basestring)):
                raise TypeError("Please pass a string for str_val")
        elif bool_val is not None:
            if not isinstance(bool_val, bool):
                raise TypeError("Please pass a bool for bool_val")
        elif int_arr_val is not None:
            if not isinstance(int_arr_val, list):
                raise TypeError("Please pass a list as int_arr_val")
        elif bool_arr_val is not None:
            if not isinstance(bool_arr_val, list):
                raise TypeError("Please pass a list as bool_arr_val")
        elif container_arr_val is not None:
            if not isinstance(container_arr_val, list):
                raise TypeError("Please pass a list as container_arr_val")
        elif str_arr_val is not None:
            if not isinstance(str_arr_val, list):
                raise TypeError("Please pass a list as str_arr_val")

        self.name = name
        self.val = val
        self.float_val = float_val
        self.str_val = str_val
        self.bool_val = bool_val
        self.int_arr_val = int_arr_val
        self.bool_arr_val = bool_arr_val
        self.container_arr_val = container_arr_val
        self.str_arr_val = str_arr_val

    def __str__(self):
        """@brief helper object printer to view the Tuple
        """
        msg = "DataTuple ->\n"
        msg += DataTuple._print_tuple(self, 1, self.name)
        return msg

    @staticmethod
    def _print_tuple(data_tuple, base_tab_spaces, base_name):
        msg = ""
        msg += "\t"*(base_tab_spaces-1) + "name = " + base_name + "\n"
        msg += "\t"*base_tab_spaces + "val = " + str(data_tuple.val) + " "+ str(type(data_tuple.val)) + "\n"
        msg += "\t"*base_tab_spaces + "float_val = " + str(data_tuple.float_val) + "\n"
        msg += "\t"*base_tab_spaces + "str_val = " + str(data_tuple.str_val) + "\n"
        msg += "\t"*base_tab_spaces + "bool_val = " + str(data_tuple.bool_val) + "\n"
        msg += "\t"*base_tab_spaces + "int_arr_val = " + str(data_tuple.int_arr_val) + "\n"
        msg += "\t"*base_tab_spaces + "bool_arr_val = " + str(data_tuple.bool_arr_val) + "\n"
        msg += "\t"*base_tab_spaces + "str_arr_val = " + str(data_tuple.str_arr_val) + "\n"
        if data_tuple.container_arr_val is not None:
            for field_name, data_tuple_con in data_tuple.container_arr_val.items():
                msg += DataTuple._print_tuple(data_tuple_con, base_tab_spaces+1, field_name)
        return msg

    def __type(self):
        return (type(self.name),
                type(self.val),
                type(self.float_val),
                type(self.str_val),
                type(self.bool_val),
                type(self.int_arr_val),
                type(self.bool_arr_val),
                type(self.str_arr_val),
                type(self.container_arr_val))

    def __key(self):
        return (self.name,
                self.val,
                self.float_val,
                self.str_val,
                self.bool_val,
                self.int_arr_val,
                self.bool_arr_val,
                self.str_arr_val,
                self.container_arr_val)

    def __hash__(self):
        return hash((self.name,
                    str(self.val),
                    str(self.float_val),
                    self.str_val,
                    self.bool_val,
                    str(self.int_arr_val),
                    str(self.bool_arr_val),
                    str(self.str_arr_val),
                    str(self.container_arr_val)))

    def __eq__(self, other):
        return (self.__key() == other.__key())
    def __lt__(self, other):
        return (self.__key() < other.__key())

@total_ordering
class KeyTuple:
    """@brief Class to create a KeyTuple. At a time only one of the following cobinations can be set ->
        (value), (value, mask), (value, prefix_len) (low, high) (value, is_valid).
        Where each combination corresponds
        to Exact, Ternary, LPM, Range and Optional match_type resectively.
        Since this class has been kept separate from the _Table class, it doesn't perform
        sanity checks along the lines of being in parity with whether the types match with that
        of the table or not. It helps to make an independent "field"
    """
    def __init__(self, name, value=None, mask=None,
            prefix_len=None, low=None, high=None, is_valid=None):
        """@brief Create a KeyTuple Object. A list of these objects are taken in as parameters to
            the constructor of Key object. Each of the params apart from the name and prefix_len
            can either be an int or a ByteArray. Any other format of inputs are not supported.
            All padding and truncation according to actual field size is done automatically as
            part of make_key.

            @param name Name of the field
            @param value Value (Integer/Long or bytearray) Accepts Strings also but only in case of some
            special custom annotations like ipv4, mac. If a custom annotation is not set, then
            an error will be thrown while trying to call make_data with string value
            @param mask Mask (Integer/Long or bytearray) Accepts Strings subject to conditions like
            value
            @param prefix_len Prefix length (Integer/Long)
            @param low low of range (Integer/Long or bytearray)
            @param high high of range (Integer/Long or bytearray)
            @param is_valid For optional match-type. Whether field is valid (bool)
            @exception ValueError Raised when a wrong combination of params are passed
            @exception TypeError Raised when an invalid type of a param is passed
        """
        self.match_type = ""
        if value is not None:
            assert low is None and high is None
            if not isinstance(value, int) and not isinstance(value, bytearray)\
                    and not isinstance(value, str)\
                    and not (is_python2() and isinstance(value, basestring))\
                    and not (is_python2() and isinstance(value, long)):
                raise TypeError("Expected type int/long, ByteArray or a string")
            self.match_type = "Exact"

        if value is not None and mask is not None:
            assert prefix_len is None
            assert low is None and high is None
            if not isinstance(value, int) and not isinstance(value, bytearray)\
                    and not isinstance(value, str)\
                    and not (is_python2() and isinstance(value, basestring))\
                    and not (is_python2() and isinstance(value, long)):
                raise TypeError("Expected type int/long, ByteArray or a string")
            if not isinstance(mask, int) and not isinstance(mask, bytearray)\
                    and not isinstance(mask, str)\
                    and not (is_python2() and isinstance(mask, basestring))\
                    and not (is_python2() and isinstance(mask, long)):
                raise TypeError("Expected type int/long, ByteArray or a string")
            self.match_type = "Ternary"

        if value is not None and prefix_len is not None:
            assert mask is None
            assert low is None and high is None
            if not isinstance(value, int) and not isinstance(value, bytearray)\
                    and not isinstance(value, str)\
                    and not (is_python2() and isinstance(value, basestring))\
                    and not (is_python2() and isinstance(value, long)):
                raise TypeError("Expected type int/long, ByteArray or a string")
            if not isinstance(prefix_len, int):
                raise TypeError("Expected type int/long")
            self.match_type = "LPM"

        if low is not None and high is not None:
            assert value is None and mask is None and prefix_len is None
            if not isinstance(low, int) and not isinstance(low, bytearray)\
                    and not isinstance(low, str)\
                    and not (is_python2() and isinstance(low, basestring))\
                    and not (is_python2() and isinstance(low, long)):
                raise TypeError("Expected type int/long or ByteArray")
            if not isinstance(high, int) and not isinstance(high, bytearray)\
                    and not isinstance(high, str)\
                    and not (is_python2() and isinstance(high, basestring))\
                    and not (is_python2() and isinstance(high, long)):
                raise TypeError("Expected type int/long or ByteArray")
            self.match_type = "Range"

        if value is not None and is_valid is not None:
            assert mask is None and prefix_len is None
            assert low is None and high is None
            if not isinstance(value, int) and not isinstance(value, bytearray)\
                    and not isinstance(value, str)\
                    and not (is_python2() and isinstance(value, basestring))\
                    and not (is_python2() and isinstance(value, long)):
                raise TypeError("Expected type int/long, ByteArray or a string")
            if not isinstance(is_valid, bool):
                raise TypeError("Expected type bool")
            self.match_type = "Optional"

        if self.match_type == "":
            raise ValueError("Wrong values passed. Please check valid combinations of params")

        self.name = name
        self.value = value
        self.mask = mask
        self.prefix_len = prefix_len
        self.low = low
        self.high = high
        self.is_valid = is_valid

    def __str__(self):
        """@brief helper object printer to view the Tuple
        """
        msg = "KeyTuple ->" + "\n"
        msg += "\tname = " + str(self.name) + "\n"
        msg += "\tvalue = " + str(self.value) + "\n"
        msg += "\tmask = " + str(self.mask) + "\n"
        msg += "\tprefix_len = " + str(self.prefix_len) + "\n"
        msg += "\tlow = " + str(self.low) + "\n"
        msg += "\thigh = " + str(self.high) + "\n"
        msg += "\tis_valid = " + str(self.is_valid) + "\n"
        return msg

    def __type(self):
        return (type(self.name),
                type(self.value),
                type(self.mask),
                type(self.prefix_len),
                type(self.low),
                type(self.high),
                type(self.is_valid))

    def __key(self):
        return (self.name,
                self.value,
                self.mask,
                self.prefix_len,
                self.low,
                self.high,
                self.is_valid)

    def __hash__(self):
        return hash((self.name,
                str(self.value),
                str(self.mask),
                str(self.prefix_len),
                str(self.low),
                str(self.high),
                str(self.is_valid)))

    def __eq__(self, other):
        return (self.__key() == other.__key())
    def __lt__(self, other):
        return (self.__key() < other.__key())

@total_ordering
class _Key:
    """@brief Class _Key (Partially internal). This class is used to create key objects. A list of these objects are
        taken by Table APIs like entry_add and entry_del. This class while creating does some
        sanity checking on the type of the KeyTuple objects passed in.
        Additionally, this object supports the following
        1. Use it as a key in dictionary since hashing is now enabled
        2. Sort using comparison
        3. get or set using index operators. Only modification of present fields is allowed. Cannot remove
        or add any fields using index operators
    """
    def __init__(self, table, key_field_list_in):
        """@brief The init takes in the _Table object for which the Key needs to be constructed.
            It also takes in a list of KeyTuple objects.
            @param key_field_list_in List of KeyTuple objects.
        """
        if not isinstance(key_field_list_in, list):
            raise TypeError("A list of KeyTuple is expected")

        key_field_list_in.sort()
        self.field_dict = OrderedDict()
        key_name_list = table.info.key_field_name_list_get()
        # Check whether all the input key_fields exist actualy or not
        input_field_names = []
        for field_in in key_field_list_in:
            if field_in.name in key_name_list:
                _Key._fix_size_verify_type(table, field_in)
            else:
                raise KeyError("The Key field name %s doesn't exist in the Table %s" % (field_in.name, table.info.name_get()))
            input_field_names.append(field_in.name)
        # Check whether all the actual mandatory key_fields are present in input or not
        for field_name in key_name_list:
            mandatory = table.info.key_field_mandatory_get(field_name)
            if mandatory:
                assert field_name in input_field_names, "%s is mandatory and needs to be in input_list" % (field_name)
        # For all input fields, add them to our main data struct = internal orderedDict where we keep all KeyTuples
        for field_in in key_field_list_in:
            self.field_dict[field_in.name] = field_in
        self.table = table

    @staticmethod
    def _fix_size_verify_type(table, field_in):
        f_size, _ = table.info.key_field_size_get(field_in.name)
        f_type = table.info.key_field_type_get(field_in.name)
        f_match_type = table.info.key_field_match_type_get(field_in.name)
        f_annotations = table.info.key_field_annotations_get(field_in.name)

        if field_in.match_type != f_match_type:
            raise ValueError("field:%s Passed in type:%s is not equal to type of keyfield:%s"
                    % (field_in.name, field_in.match_type, f_match_type))

        if f_type == "uint64" or f_type == "bytes" or f_type == "uint32" or f_type == "uint16" or f_type == "uint8":
            if (f_match_type == "Exact"):
                field_in.value = _convert_to_bytearray(field_in.value,
                        field_in.name, f_size, f_annotations)

            elif (f_match_type == "Ternary"):
                field_in.value = _convert_to_bytearray(field_in.value,
                        field_in.name, f_size, f_annotations)
                field_in.mask = _convert_to_bytearray(field_in.mask,
                        field_in.name, f_size, f_annotations)

            elif (f_match_type == "LPM"):
                field_in.value = _convert_to_bytearray(field_in.value,
                        field_in.name, f_size, f_annotations)

                # Check if prefix len is greater than the size in bytes
                assert (f_size*8) >= field_in.prefix_len,\
                    "Field %s Prefix length %d passed in is > size %d"\
                    % (field_in.name, field_in.prefix_len, f_size*8)

            elif (f_match_type == "Range"):
                field_in.low = _convert_to_bytearray(field_in.low,
                        field_in.name, f_size, f_annotations)
                field_in.high = _convert_to_bytearray(field_in.high,
                        field_in.name, f_size, f_annotations)

            elif (f_match_type == "Optional"):
                field_in.value = _convert_to_bytearray(field_in.value,
                        field_in.name, f_size, f_annotations)

    def to_dict(self):
        """@brief Converts this object them to a readable dictionary with keys as field_names and
        values as the values. The values are in a human readable format.
            @return dictionary (field_names -> field_values)
        """
        ret_dict = {}
        for field in list(self.field_dict.values()):
            ret_dict[field.name] = self._get_val(field)
        return ret_dict

    def apply_mask(self):
        """@brief Goes over all the fields and applies mask according to the prefix_len or
        mask of the field for LPM and Ternary fields respectively.
        This is useful in cases where the device returns a masked value as part of a get request
        but the sent value might have contained unmasked values. This function makes it easier
        to perform comparisons by applying the mask on the sent key as well.
        For example, sent key might contain value = 128.0.0.1/24 but returned key will have
        128.0.0.0/24
        """
        def _mask_from_mask(value, mask):
            return bytearray(b1 & b2 for b1, b2 in zip(value, mask))

        def _mask_from_prefix_len(value, prefix_len, bits):
            byte_size = (bits+7)//8
            ret = bytearray(byte_size)
            if not prefix_len:
                return ret
            mask = 1
            for i in range(prefix_len-1):
                mask = mask<<1
                mask |= 1
            for i in range(bits - prefix_len):
                mask = mask<<1
            return _mask_from_mask(value, to_bytes(mask, byte_size))

        for field in list(self.field_dict.values()):
            f_size, f_bits = self.table.info.key_field_size_get(field.name)
            f_type = self.table.info.key_field_type_get(field.name)
            f_match_type = self.table.info.key_field_match_type_get(field.name)
            f_annotations = self.table.info.key_field_annotations_get(field.name)

            if f_type == "uint64" or f_type == "bytes" or f_type == "uint32" or f_type == "uint16" or f_type == "uint8":
                if (f_match_type == "Ternary"):
                    field.value = _mask_from_mask(field.value, field.mask)

                elif (f_match_type == "LPM"):
                    field.value = _mask_from_prefix_len(field.value, field.prefix_len, f_bits)

    def _get_val(self, field_in):
        f_size, _ = self.table.info.key_field_size_get(field_in.name)
        f_type = self.table.info.key_field_type_get(field_in.name)
        f_match_type = self.table.info.key_field_match_type_get(field_in.name)
        f_annotations = self.table.info.key_field_annotations_get(field_in.name)

        key_dict = {}
        if f_type == "uint64" or f_type == "bytes" or f_type == "uint32" or f_type == "uint16" or f_type == "uint8":
            if (f_match_type == "Exact"):
                key_dict["value"] = _convert_to_presentation(field_in.value,
                        field_in.name, f_size, f_annotations)

            elif (f_match_type == "Ternary"):
                key_dict["value"] = _convert_to_presentation(field_in.value,
                        field_in.name, f_size, f_annotations)
                key_dict["mask"] = _convert_to_presentation(field_in.mask,
                        field_in.name, f_size, f_annotations)

            elif (f_match_type == "LPM"):
                key_dict["value"] = _convert_to_presentation(field_in.value,
                        field_in.name, f_size, f_annotations)
                key_dict["prefix_len"] = _convert_to_presentation(field_in.prefix_len,
                        field_in.name, f_size, f_annotations)

            elif (f_match_type == "Range"):
                key_dict["low"] = _convert_to_presentation(field_in.low,
                        field_in.name, f_size, f_annotations)
                key_dict["high"] = _convert_to_presentation(field_in.high,
                        field_in.name, f_size, f_annotations)

            elif (f_match_type == "Optional"):
                key_dict["value"] = _convert_to_presentation(field_in.value,
                        field_in.name, f_size, f_annotations)
                key_dict["is_valid"] = field_in.is_valid

        elif f_type == "string":
                key_dict["value"] = str(field_in.value)
        return key_dict

    def __contains__(self, key):
        return key in self.field_dict
    def __getitem__(self, key):
        return self.field_dict[key]
    def __setitem__(self, key, val):
        assert isinstance(val, KeyTuple), "%s is not an instance of KeyTuple."\
                "Received type =%s" %(key, type(val))
        if (key != val.name):
            raise KeyError("Passed in name %s doesn't match tuple name %s" %(key, val.name))
        if key in self.field_dict:
            _Key._fix_size_verify_type(self.table, val)
            self.field_dict[key] = val
        else:
            raise KeyError("%s has not already been set as part of this Object."
                " Please create the correct object using make_key first."
                " The index operator can only be used to get or modify an existing field"%(key))
    def __delitem__(self, key):
        raise RuntimeError("Cannot delete a field. Can only get or modify an existing field")

    def __str__(self):
        return str(self.to_dict())
    def __key(self):
        return list(self.field_dict.values())
    def __hash__(self):
        return hash(tuple(self.__key()))
    def __eq__(self, other):
        return (self.__key() == other.__key())
    def __lt__(self, other):
        return (self.__key() < other.__key())

@total_ordering
class _Data:
    """@brief Class _Data (Partially internal). This class is used to create data objects. A list of these objects are
        taken by Table APIs like entry_add and entry_del. This class while creating does some
        sanity checking on the type of the DataTuple objects passed in. This class can also be used
        to create Data objects needed to be given to filtered entry_get.
        Additionally, this object supports the following
        1. Use it as a key in dictionary since hashing is now enabled
        2. Sort using comparison
        3. get or set fields in this object using index operators.
        Only modification of present fields is allowed. Cannot remove or add any fields using index operators
        Please note that the index operator on this object itself,
        can be used to get any corresponding DataTuple, which is different from the dict returned by to_dict(). to_dict() returns
        just a presentation dictionary with actual values as the data values and it contains 2 additional keys called
        is_default_entry and action_name. In contrast, the index operator on this object returns the DataTuple objects for field names.
    """
    def __init__(self, obj, data_field_list_in=[], action_name=None, get=False):
        """@brief The init takes in the _Table/_Learn object for which the Data needs to be constructed.
            It also takes in a list of DataTuple objects.
            @param data_field_list_in List of DataTuple objects.
            @param action_name (optional) The action name. If a table contains an action but no action_name
            is given, then the field wil be only searched for in the common data fields.
            @param get (optional) If a Data object needs to be constructed for get, then having
            DataTuple objects which have only names populated is completely valid. Hence size
            checking is skipped in the ctor if this is True
        """
        if not isinstance(data_field_list_in, list):
            raise TypeError("A list of DataTuple is expected")

        data_name_list = obj.info.data_field_name_list_get(action_name)
        # Double loop over input_list and the actual fields. Check whether the passed
        # in size is equal to the actual size of the fields
        if not get:
            for field_in in data_field_list_in:
                if field_in is None:
                    continue
                if field_in.name in data_name_list:
                    try:
                        _Data._verify_size_and_set(obj, action_name, field_in)
                    except ValueError as e:
                        logger.error("Field %s for action %s not found; check if \"get\" should have been True", field_in.name, action_name)
                        raise e
                else:
                    KeyError("The Data field name %s doesn't exist in the Table %s" % (field_in.name,
                            obj.info.name_get()))
        field_list_in = _Data._check_for_dups(obj, action_name, data_field_list_in)
        field_list_in.sort()
        self.field_dict = OrderedDict()
        for field_in in field_list_in:
            self.field_dict[field_in.name] = field_in
        self.action_name = action_name
        self.is_default_entry = False
        self.obj = obj

    @staticmethod
    def _check_for_dups(obj, action_name, data_field_list_in):
        seen_names = dict()
        for field_in in data_field_list_in:
            if field_in is None:
                continue
            f_annotations = obj.info.data_field_annotations_get(field_in.name, action_name)
            if field_in.name in seen_names:
                # if it is a register field and already this field ID is seen,
                # then squash this one in the previously seen field
                if "$bfrt_field_class.register_data" in f_annotations:
                    seen_names[field_in.name].int_arr_val.append(field_in.val)
                else:
                    raise KeyError("Duplicate field %s found" %(field_in.name))
            else:
                seen_names[field_in.name] = field_in
                # Add to int_arr_val in case of register to make sure
                # that values returned for single pipe are handled as
                # single element array not just integer value
                if ("$bfrt_field_class.register_data" in f_annotations and
                    field_in.val != None):
                    seen_names[field_in.name].int_arr_val = [field_in.val]
        return list(seen_names.values())


    @staticmethod
    def _verify_size_and_set(obj, action_name, field_in):
        """@brief This function does the following on a field
            1. Convert it to a bytearray from either an int or a
                string (custom annotations)
            2. Check -> if it is mandatory, then it should not be None
            3. Check -> if the field size is equal to the final bytearray
                or not. Only done for uints/bytes
        """
        def check_exists(f_name, member_label, val_to_check, f_mandatory):
            if f_mandatory and val_to_check is None:
                raise ValueError("mandatory = %s %s = %s field %s"\
                    % (f_mandatory, member_label, val_to_check, f_name))
        def check_size(f_name, f_size, size_to_check):
            if f_size != size_to_check:
                raise ValueError("expected %d len received %d len for field %s"\
                    % (f_size, size_to_check, f_name))

        f_size, _ = obj.info.data_field_size_get(field_in.name, action_name)
        f_type = obj.info.data_field_type_get(field_in.name, action_name)
        f_repeated = obj.info.data_field_repeated_get(field_in.name, action_name)
        f_mandatory = obj.info.data_field_mandatory_get(field_in.name, action_name)
        f_annotations = obj.info.data_field_annotations_get(field_in.name, action_name)
        if not f_repeated:
            if f_type == "uint64" or f_type == "bytes"\
                    or f_type == "uint32" or f_type == "uint16"\
                    or f_type == "uint8":
                field_in.val = _convert_to_bytearray(field_in.val,
                        field_in.name, f_size, f_annotations)
                check_exists(field_in.name, "field.val", field_in.val, f_mandatory)
                if field_in.val is not None:
                    check_size(field_in.name, f_size, len(field_in.val))
            elif f_type == "bool":
                check_exists(field_in.name, "field.bool_val", field_in.bool_val, f_mandatory)
            elif f_type == "float":
                check_exists(field_in.name, "field.float_val", field_in.float_val, f_mandatory)
            elif f_type == "string":
                check_exists(field_in.name, "field.str_val", field_in.str_val, f_mandatory)
            else:
                raise TypeError("Field %s Wrong type %s encountered" % (field_in.name), f_type)
        else:
            if f_type == "uint64" or f_type == "bytes"\
                    or f_type == "uint32" or f_type == "uint16"\
                    or f_type == "uint8":
                check_exists(field_in.name, "field.int_arr_val", field_in.int_arr_val, f_mandatory)
            elif f_type == "bool":
                check_exists(field_in.name, "field.bool_arr_val", field_in.bool_arr_val, f_mandatory)
            elif f_type == "string":
                check_exists(field_in.name, "field.str_arr_val", field_in.str_arr_val, f_mandatory)

            elif f_type == "container":
                for container_dict in field_in.container_arr_val:
                    for field_name_c, field_c in container_dict.items():
                        _Data._verify_size_and_set(obj, None, field_c)
            else:
                raise TypeError("Field %s Wrong type encountered" % (field_in.name), f_type)

    def to_dict(self):
        """@brief Converts this object them to a readable dictionary with keys as field_names and
            values as the values. The values are in a human readable format. data_dict will contain a
            field called "action_name" as well, containing
            the action_name. It will also contain a field called "is_default_entry" which will be a bool
            indicating whether this object is a default_entry_data
            @return Dictionary (field_names -> Field_values)
        """
        ret_dict = {}
        for field in list(self.field_dict.values()):
            ret_dict[field.name] = self._get_val(field)
        ret_dict["action_name"] = self.action_name
        ret_dict["is_default_entry"] = self.is_default_entry
        return ret_dict

    def _get_val(self, field_in):
        f_size, _ = self.obj.info.data_field_size_get(field_in.name, self.action_name)
        f_type = self.obj.info.data_field_type_get(field_in.name, self.action_name)
        f_repeated = self.obj.info.data_field_repeated_get(field_in.name, self.action_name)
        f_annotations = self.obj.info.data_field_annotations_get(field_in.name, self.action_name)
        if not f_repeated:
            if f_type == "uint64" or f_type == "bytes" or\
                    f_type == "uint32" or f_type == "uint16"\
                    or f_type == "uint8":
                # if it is a register field, then return the internal array.
                # if an exception occurs, then the object must be a data object
                # for entry_add() which will contain register value in field_in.val
                if "$bfrt_field_class.register_data" in f_annotations:
                    if field_in.int_arr_val is not None:
                        return [_convert_to_presentation(x,
                            field_in.name, f_size, f_annotations) for x in field_in.int_arr_val]
                    else:
                        return _convert_to_presentation(field_in.val,
                                field_in.name, f_size, f_annotations)
                return _convert_to_presentation(field_in.val,
                        field_in.name, f_size, f_annotations)
            elif f_type == "bool":
                return field_in.bool_val
            elif f_type == "float":
                return field_in.float_val
            elif f_type == "string":
                return field_in.str_val
            else:
                raise TypeError("Field %s Wrong type %s encountered" % (field_in.name), f_type)
        else:
            if f_type == "uint64" or f_type == "bytes" or\
                    f_type == "uint32" or f_type == "uint16" or\
                    f_type == "uint8":
                return field_in.int_arr_val
            elif f_type == "bool":
                return field_in.bool_arr_val
            elif f_type == "string":
                return field_in.str_arr_val
            elif f_type== "container":
                return_list = []
                for container_dict in field_in.container_arr_val:
                    dict_con = {}
                    for field_name_c, field_c in container_dict.items():
                        dict_con[field_name_c] = self._get_val(field_c)
                    return_list.append(dict_con)
                return return_list
            else:
                raise TypeError("Field %s Wrong type encountered" % (field_in.name), f_type)
        return None

    def __contains__(self, key):
        return key in self.field_dict
    def __getitem__(self, key):
        return self.field_dict[key]
    def __setitem__(self, key, val):
        assert isinstance(val, DataTuple), "%s is not an instance of DataTuple."\
                "Received type =%s" %(key, type(val))
        if (key != val.name):
            raise KeyError("Passed in name %s doesn't match tuple name %s" %(key, val.name))
        if key in self.field_dict:
            _Data._verify_size_and_set(self.obj, self.action_name, val)
            self.field_dict[key] = val
        else:
            raise KeyError("%s has not already been set as part of this Object."
                " Please create the correct object using make_data first."
                " The index operator can only be used to get or modify an existing field"%(key))
    def __delitem__(self, key):
        raise RuntimeError("Cannot delete a field. Can only get or modify an existing field")

    def __str__(self):
        return str(self.to_dict())
    def __key(self):
        return (tuple(self.field_dict.values()), self.action_name)
    def __hash__(self):
        return hash(self.__key())
    def __eq__(self, other):
        return (self.__key() == other.__key())
    def __lt__(self, other):
        return (self.__key() < other.__key())

class _GetParser:
    """@brief (Internal)
    """
    def __init__(self, obj):
        self.obj = obj

    def _parse_table_usage(self, response):
        '''
        table_id_dict is a dictionary of table_ids
        '''
        for rep in response:
            for entity in rep.entities:
                yield entity.table_usage.usage
                # Yielding here allows to iterate over more entities

    def _parse_key(self, key):
        key_tuple_list = []
        for key_field in key.fields:
            value = mask = prefix_len = low = high = is_valid = None
            if key_field.HasField("exact"):
                value = bytearray(key_field.exact.value)
            elif key_field.HasField("ternary"):
                value = bytearray(key_field.ternary.value)
                mask = bytearray(key_field.ternary.mask)
            elif key_field.HasField("lpm"):
                value = bytearray(key_field.lpm.value)
                prefix_len = key_field.lpm.prefix_len
            elif key_field.HasField("range"):
                low = bytearray(key_field.range.low)
                high = bytearray(key_field.range.high)
            elif key_field.HasField("optional"):
                value = bytearray(key_field.optional.value)
                is_valid = key_field.optional.is_valid
            name = self.obj.info.key_field_name_get(key_field.field_id)
            key_tuple_list.append(KeyTuple(name, value, mask, prefix_len, low, high, is_valid))
        return self.obj.make_key(key_tuple_list)

    def _parse_target(self, tgt):
        # Fields with value 0 are stripped from message, hence initialize all.
        data = {'dev_id':0, 'pipe_id':0, 'direction':0, 'prsr_id':0}
        for tgt_field in tgt.ListFields():
            name = tgt_field[0].name
            value = tgt_field[1]
            data[name] = value
        return Target(data['dev_id'], data['pipe_id'], data['direction'], data['prsr_id'])

    def _parse_data_field(self, field, action_name):
        '''@brief Parse a received DataField
        '''
        val = float_val = str_val = bool_val = int_arr_val = bool_arr_val = None
        str_arr_val = container_arr_val = None
        name = ""
        if action_name is None:
            name = self.obj.info.data_field_name_get(field.field_id)
        else:
            name = self.obj.info.data_field_name_get(field.field_id, action_name)

        if field.HasField("stream"):
            val = bytearray(field.stream)
        elif field.HasField("float_val"):
            float_val = float(field.float_val)
        elif field.HasField("str_val"):
            str_val = str(field.str_val)
        elif field.HasField("bool_val"):
            bool_val = bool(field.bool_val)
        elif field.HasField("int_arr_val"):
            int_arr_val = [int(x) for x in field.int_arr_val.val]
        elif field.HasField("bool_arr_val"):
            bool_arr_val = [bool(x) for x in field.bool_arr_val.val]
        elif field.HasField("str_arr_val"):
            str_arr_val = [str(x) for x in field.str_arr_val.val]
        elif field.HasField("container_arr_val"):
            # DataField objects could be encapsulated within a DataField object.
            # Parse the inner dataField objects here.
            # Lets say we have 12 repeated containers of field_info container which has
            # 32 fields each. then we will insert 12 dictionaries in the container_arr_list
            container_arr_val = []
            for container in field.container_arr_val.container:
                data_fields = {}
                for field_con in container.val:
                    field_name = self.obj.info.data_field_name_get(field_con.field_id)
                    data_fields[field_name] = self._parse_data_field(field_con, None)
                container_arr_val.append(data_fields)
        else:
            # None of the OneOf fields were set. It is just a NoneVal. Log and ignore.
            logger.debug("Encountered unknown type while data parsing response. name = %s" %(name))
        return DataTuple(name, val, float_val, str_val, int_arr_val, bool_arr_val,
                bool_val, container_arr_val, str_arr_val)

    def _parse_data(self, entry_data, is_default_entry):
        action_name = None
        if entry_data.action_id is not 0:
            action_name = self.obj.info.action_name_get(entry_data.action_id)
        data_tuple_list = []
        for field in entry_data.fields:
            data_tuple_list.append(self._parse_data_field(field, action_name))
        data = self.obj.make_data(data_tuple_list, action_name)
        data.is_default_entry = is_default_entry
        return data

    def _parse_attribute_get_response(self, response):
        try:
            for rep in response:
                for entity in rep.entities:
                    ret_dict = {}
                    attr = entity.table_attribute
                    table_id = attr.table_id
                    assert self.obj.info.id_get() == table_id, \
                    "Table ids do not match, table ID received %d, table ID of the obj = %d" %(table_id, self.obj.info.id_get())

                    if attr.HasField("idle_table"):
                        ret_dict["ttl_query_interval"] = attr.idle_table.ttl_query_interval
                        ret_dict["max_ttl"] = attr.idle_table.max_ttl
                        ret_dict["min_ttl"] = attr.idle_table.min_ttl
                        ret_dict["idle_table_mode"] = attr.idle_table.idle_table_mode
                        ret_dict["enable"] = attr.idle_table.enable
                    elif attr.HasField("entry_scope"):
                        ret_dict["gress_scope"] = self._parse_entry_scope_mode(attr.entry_scope.gress_scope)
                        ret_dict["pipe_scope"] = self._parse_entry_scope_mode(attr.entry_scope.pipe_scope)
                        ret_dict["prsr_scope"] = self._parse_entry_scope_mode(attr.entry_scope.prsr_scope)
                    elif attr.HasField("dyn_hashing"):
                        ret_dict["alg"] = attr.dyn_hashing.alg
                        ret_dict["seed"] = attr.dyn_hashing.seed
                    elif attr.HasField("byte_count_adj"):
                        ret_dict["byte_count_adjust"] = attr.byte_count_adj.byte_count_adjust
                    elif attr.HasField("port_status_notify"):
                        ret_dict["enable"] = attr.port_status_notify.enable
                    elif attr.HasField("intvl_ms"):
                        ret_dict["intvl_val"] = attr.intvl_ms.intvl_val
                    yield ret_dict
        except grpc.RpcError as e:
            raise BfruntimeReadWriteRpcException(e)

    def _parse_dkm(self, fields):
        key_tuple_list = []
        for field in fields:
            name = self.obj.info.key_field_name_get(field.field_id)
            key_tuple_list.append(KeyTuple(name, bytearray(field.mask)))
        return self.obj.make_key(key_tuple_list)


    def _parse_entry_scope_mode(self, scope_mode):
        mode_dict = {}
        if scope_mode.HasField("predef"):
            mode_dict["predef"] = scope_mode.predef
        elif scope_mode.HasField("user_defined"):
            mode_dict["user_defined"] = scope_mode.user_defined
        mode_dict["args"] = scope_mode.args
        return mode_dict

    def _parse_handle_get_response(self, response):
        try:
            for rep in response:
                for entity in rep.entities:
                    assert entity.handle.HasField("key") == False, \
                    "Response to get_handle has key, while should have only handle"
                    handle = entity.handle.handle_id
                    return handle
        except grpc.RpcError as e:
            raise BfruntimeReadWriteRpcException(e)

    def _parse_entry_get_response(self, response, get_default_entry=False):
        try:
            for rep in response:
                for entity in rep.entities:
                    data = key = tgt = None
                    table_id = entity.table_entry.table_id
                    if entity.table_entry.HasField("entry_tgt"):
                        tgt = self._parse_target(entity.table_entry.entry_tgt)
                    if entity.table_entry.HasField("key"):
                        key = self._parse_key(entity.table_entry.key)
                        assert table_id == key.table.info.id_get(),\
                        "Table ids do not match, table ID received %d, table ID of key obj = %d" %(table_id, key.table.info.id_get())
                    if entity.table_entry.HasField("data"):
                        data = None
                        # Skip parsing default entry when not required
                        if (entity.table_entry.is_default_entry and get_default_entry) or \
                            (not entity.table_entry.is_default_entry and not get_default_entry):
                            data = self._parse_data(entity.table_entry.data,
                                    entity.table_entry.is_default_entry)
                            assert table_id == data.obj.info.id_get(),\
                            "Table ids do not match, table ID received %d,\
                            table ID of data obj = %d" %(table_id, data.obj.info.id_get())
                    # If Both data and Key are None, then don't yield, just continue
                    if data is None and key is None:
                        continue
                    # Yielding here allows to iterate over more entities
                    if tgt is not None:
                        yield data, key, tgt
                    else:
                        yield data, key
        except grpc.RpcError as e:
            raise BfruntimeReadWriteRpcException(e)

class _Table:
    """@brief Class _Table (Partially internal). Objects of this class are created during BfRtInfo Parsing and do not need
        to be created separately. These objects can be queried from a _BfRtInfo class using a table_get()
        call. Contains an interface to interact with a table. In order to
        access table metadata associated with it, one needs to access its _TableInfo object(info). All other
        functions related to a table are on this class itself.
        Most public functions accept an optional 'metadata' parameter which is not be confused with table/learn obj
        metadata. The optional parameter is to send various signals like
        1. Setting a timeout for the server to stop
        processing a write/read request in the backend. This might be required in a scenario where a client is tired of
        waiting after sending an annoyingly long request and wants an option to let the server know beforehand
        to stop after a certain time. By default, there is not timeout and the server continues processing till end
        of batch.
        Format ::
        metadata = (("deadline_sec", "1"), ("deadline_nsec", "999999999"))
    """
    def __init__(self, table_info, reader_writer_interface):
        """@brief Internal function. Used by _BfRtInfo to create a table
            @param table_info _TableInfo object
            @param reader_writer_interface This object is used to Read and write upon
        """
        self.info = table_info
        self.reader_writer_interface = reader_writer_interface
        self.get_parser = _GetParser(self)

    def make_key(self, key_field_list_in):
        """@brief Create a _Key object using a list of KeyTuple
            @param key_field_list_in list of KeyTuple.
            @return _Key object
        """
        return _Key(self, key_field_list_in)

    def make_data(self, data_field_list_in=[], action_name=None, get=False):
        """@brief Create a _Data object using a list of DataTuple
            @param data_field_list_in List of DataTuple objects.
            @param action_name (optional) The action name. If a table contains an action but no action_name
            is given, then the field wil be only searched for in the common data fields.
            @param get (optional) If a Data object needs to be constructed for get, then having
            DataTuple objects which have only names populated is completely valid. Hence size
            checking is skipped in the ctor if this is True
        """
        return _Data(self, data_field_list_in, action_name, get)

    def entry_add(self, target, key_list=None, data_list=None,
            atomicity=bfruntime_pb2.WriteRequest.CONTINUE_ON_ERROR, p4_name=None,
            metadata=None):
        """@brief Insert table entries. len of key_list, data_list and action_name_list
            should be equal
            @param target target device
            @param key_list List of Keys.
            @param data_list List of Data. Each Data object contains action_name info as well
            @param atomicity Defines atomicity of add requests. Default is CONTINUE_ON_ERROR
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """

        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        req.atomicity = atomicity
        return self.reader_writer_interface._write(self._entry_write_req_make(req, key_list,
            data_list, bfruntime_pb2.Update.INSERT), metadata)

    def entry_mod(self, target, key_list=None, data_list=None, flags={"reset_ttl":True}, p4_name=None, metadata=None):
        """@brief Modify table entries. len of key_list, data_list and action_name_list
            should be equal
            @param target target device
            @param flags dictionary of modify flags.
            @param key_list List of Keys.
            @param data_list List of Data. Each Data object contains action_name info as well
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """

        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        return self.reader_writer_interface._write(self._entry_write_req_make(req, key_list,
            data_list, bfruntime_pb2.Update.MODIFY, flags=flags), metadata)

    def entry_mod_inc(self, target,
                           key_list=None, data_list=None,
                           modify_inc_type=bfruntime_pb2.TableModIncFlag.MOD_INC_ADD,
                           p4_name=None, metadata=None):
        """@brief Modify table entries. len of key_list, data_list and action_name_list
            should be equal
            @param target target device
            @param key_list List of Keys.
            @param data_list List of Data. Each Data object contains action_name info as well
            @param modify_inc_type Type of incremental modify. Can be Del or add
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """

        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        return self.reader_writer_interface._write(self._entry_write_req_make(req, key_list,
            data_list, bfruntime_pb2.Update.MODIFY_INC, modify_inc_type), metadata)

    def entry_del(self, target, key_list=None, p4_name=None, metadata=None):
        """@brief Delete table entries.
            @param target target device
            @param key_list List of Keys.
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        # Following just ensures that self._entry_write_req_make will make at least
        # one request
        if key_list == None or key_list == []:
            key_list = [None]
        return self.reader_writer_interface._write(self._entry_write_req_make(req, key_list,
            None, bfruntime_pb2.Update.DELETE), metadata)

    def default_entry_set(self, target, data, p4_name=None, metadata=None):
        """ @brief Set default entry
            @param target target device
            @param data Data object
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """

        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.MODIFY

        table_entry = update.entity.table_entry
        table_entry.table_id = self.info.id_get()
        table_entry.is_default_entry = True

        data.is_default_entry = True

        self._set_table_data(table_entry, data.action_name, list(data.field_dict.values()))
        return self.reader_writer_interface._write(req, metadata)

    def default_entry_reset(self, target, p4_name=None, metadata=None):
        """@brief Reset default entry
           @param target target device
           @param p4_name string containing name of P4 with which to communicate
           optional for subscribed clients, it is mainly used for independent clients
           who want to communicate with a p4 without having to subscribe
           @param metadata : optional metadata to send with write request
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        update = req.updates.add()
        update.type = bfruntime_pb2.Update.DELETE
        table_entry = update.entity.table_entry
        table_entry.table_id = self.info.id_get()
        table_entry.is_default_entry = True

        return self.reader_writer_interface._write(req, metadata)

    def default_entry_get(self, target, flags={"from_hw":True}, required_data=None, p4_name=None, metadata=None):
        """@brief Get default entry
            @param target target device
            @param flags dictionary of read flags.
            @param required_data (optional) Data object containing info regarding
            data fields and actions which are of interest. The actual action_name associated with a
            field is returned as part of the returned object. But if action_name is populated here, then
            the request contains an action name. The server then checks whether the key requested,
            if exists, is associated with this action or not. The get fails in case of a discrepancy.
            The mentioned data fields indicate if only some particular
            fields are required from the server as part of the data. All other data fields are not
            returned by the server. This entire parameter acts as a filter.
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with read request
            @return generator object on which one can iterate on to get (Data, None).
        """
        req = bfruntime_pb2.ReadRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        resp = self.reader_writer_interface._read(self._entry_read_req_make(req, None, flags,
            required_data, True), metadata)
        return self.get_parser._parse_entry_get_response(resp, get_default_entry=True)


    def entry_get(self, target,
            key_list=None, flags={"from_hw":True}, required_data=None, handle=None, p4_name=None, metadata=None):
        """@brief Get table entries. An empty key_list indicates get all entries
            @param target target device
            @param key_list List of Keys. In other words, List of list of KeyTuples.
            @param flags dictionary of read flags.
            @param required_data (optional) Data object containing info regarding
            data fields and actions which are of interest. The actual action_name associated with a
            field is returned as part of the returned object. But if action_name is populated here, then
            the request contains an action name. The server then checks whether the key requested,
            if exists, is associated with this action or not. The get fails in case of a discrepancy.
            The mentioned data fields indicate if only some particular
            fields are required from the server as part of the data. All other data fields are not
            returned by the server. This entire parameter acts as a filter.
            @param metadata : optional metadata to send with read request
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param handle : optional entry handle to fetch entry by handle.
            Key will be ignored in such case when fetching data.

            @return A generator object on which one can iterate to get a pair of (Data, Key).
            Each iteration will give a _Data and a _Key object for each entry. On the
            _Data and _Key objects, to_dict() function can be called to convert them to a readable
            dictionary. data_dict will contain a field called "action_name" as well, containing
            the action_name. It will also contain a field called "is_default_entry" which will be a bool
            indicating whether this object is a default_entry_datai. If key_only
            flag is set to True, instead of returning _Data and _Key, generator
            will return _Data, _Key and Target. In such case _Data object will be
            equal to None.

        """

        req = bfruntime_pb2.ReadRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        resp = self.reader_writer_interface._read(self._entry_read_req_make(req, key_list, flags,
            required_data, False, handle), metadata)
        return self.get_parser._parse_entry_get_response(resp)

    def usage_get(self, target, p4_name=None, metadata=None):
        """@brief Get current usage of the table
            @param target target device
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with read request
            @return A generator object which one can call next to get the current usage
        """
        req = bfruntime_pb2.ReadRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        table_usage = req.entities.add().table_usage
        table_usage.table_id = self.info.id_get()
        return self.get_parser._parse_table_usage(self.reader_writer_interface._read(req, metadata))

    def attribute_get(self, target, attribute_name, p4_name=None, metadata=None):
        """@brief Get attribute
            @param target target device
            @param attribute_name This is a string which should be exactly the one which
            is present as part of 'attributes' in the bf-rt.json. The supported attributes on
            the table can be retrieved using info.attributes_supported_get(). To list down a few
            a. EntryScope
            b. IdleTimeout
            d. DynamicHashing
            e. MeterByteCountAdjust
            f. port_status_notif_cb
            g. poll_intvl_ms
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with read request
            @return A generator object which one can call next to get the attribute. A dictionary
            is returned which contains attribute get replies. The object type in the dictionary
            differs for each attribute
            a. EntryScope = {"gress_scope": {"predef":<int>, "user_defined":<int>, "args":<int>},
                             "pipe_scope": {"predef":<int>, "user_defined":<int>, "args":<int>},
                             "prsr_scope": {"predef":<int>, "user_defined":<int>, "args":<int>}}
            b. IdleTimeout = {"ttl_query_interval": <int>, "max_ttl": <int> , ....}
            d. DynamicHashing = {"alg": <int>, "seed":<int>}
            e. MeterByteCountAdjust = {"byte_count_adjust": <int>}
            f. port_status_notif_cb = {"enable": <bool>}
            g. poll_intvl_ms = {"intvl_val": <int>}

        """
        if attribute_name not in self.info.attributes_supported_get():
            raise KeyError("Attribute %s not in supported list %s"
                    %(attribute_name,str(self.info.attributes_supported_get())))
        req = bfruntime_pb2.ReadRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        resp = self.reader_writer_interface._read(self._attribute_read_req_make(req, attribute_name), metadata)
        return self.get_parser._parse_attribute_get_response(resp)


    def attribute_entry_scope_set(self, target, config_gress_scope = False, predefined_gress_scope_val=bfruntime_pb2.Mode.ALL, config_pipe_scope=True, predefined_pipe_scope=True, predefined_pipe_scope_val=bfruntime_pb2.Mode.ALL, user_defined_pipe_scope_val=0xffff, pipe_scope_args=0xff, config_prsr_scope = False, predefined_prsr_scope_val=bfruntime_pb2.Mode.ALL, prsr_scope_args=0xff, p4_name=None, metadata=None):
        """@brief Set Entry Scope for the table
            @param target target device
            @param config_gress_scope configure gress_scope for the table
            @param predefined_gress_scope_val (Optional) Only valid when config_gress_scope=True
            @param config_pipe_scope configure pipe_scope for the table
            @param predefined_pipe_scope (Optional) Only valid when config_pipe_scope=True, configure pipe_scope to predefined scope or user_defined one
            @param predefined_pipe_scope_val (Optional) Only valid when config_pipe_scope=True
            @param user_defined_pipe_scope_val (Optional) Only valid when pipe_scope type is user defined
            @param pipe_scope_args (Optional) Only valid when config_pipe_scope=True
            @param config_prsr_scope configure prsr_scope for the table
            @param predefined_prsr_scope_val (Optional) Only valid when config_prsr_scope=True
            @param prsr_scope_args (Optional) Only valid when config_prsr_scope=True
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.info.id_get()

        if config_gress_scope == True:
            table_attribute.entry_scope.gress_scope.predef = predefined_gress_scope_val
        if config_pipe_scope == True:
            if predefined_pipe_scope == True:
                table_attribute.entry_scope.pipe_scope.predef = predefined_pipe_scope_val
            else:
                table_attribute.entry_scope.pipe_scope.user_defined = user_defined_pipe_scope_val
            table_attribute.entry_scope.pipe_scope.args = pipe_scope_args
        if config_prsr_scope == True:
            table_attribute.entry_scope.prsr_scope.predef = predefined_prsr_scope_val
            table_attribute.entry_scope.prsr_scope.args = prsr_scope_args

        return self.reader_writer_interface._write(req, metadata)


    def attribute_idle_time_set(self, target, enable = False, idle_table_mode = bfruntime_pb2.IdleTable.IDLE_TABLE_NOTIFY_MODE, ttl_query_interval = 5000, max_ttl = 3600000, min_ttl = 1000, p4_name=None, metadata=None):
        """ Set idletime table params
            @param target target device
            @param idle_table_mode Mode of the idle table (POLL_MODE or NOTIFY_MODE)
            @param ttl_query_interval Minimum query interval
            @param max_ttl Max TTL any entry in this table can have in msecs
            @param min_ttl Min TTL any entry in this table can have in msecs
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.info.id_get()

        table_attribute.idle_table.enable = enable
        table_attribute.idle_table.idle_table_mode = idle_table_mode
        table_attribute.idle_table.ttl_query_interval = ttl_query_interval
        table_attribute.idle_table.max_ttl = max_ttl
        table_attribute.idle_table.min_ttl = min_ttl
        return self.reader_writer_interface._write(req, metadata)

    def attribute_port_status_change_set(self, target, enable = False, p4_name=None, metadata=None):
        """@brief Set port status change notification for the table
            @param target target device
            @param enable notification enable
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.info.id_get()

        table_attribute.port_status_notify.enable = enable
        return self.reader_writer_interface._write(req, metadata)

    def attribute_port_stat_poll_intvl_set(self, target, intvl, p4_name=None, metadata=None):
        """@brief Set port stat poll interval(ms) for the table
            @param target target device
            @param intvl time interval, millisecond
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.info.id_get()

        table_attribute.intvl_ms.intvl_val = intvl
        return self.reader_writer_interface._write(req, metadata)

    def attribute_meter_bytecount_adjust_set(self, target, byte_count = 0, p4_name=None, metadata=None):
        """@brief Set meter bytecount adjust attribute for the meter table
            @param target target device
            @param byte_count number of adjust bytes
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.info.id_get()
        table_attribute.byte_count_adj.byte_count_adjust = byte_count;

        return self.reader_writer_interface._write(req, metadata)

    def operations_execute(self, target, table_op, p4_name=None, metadata=None):
        """@brief Apply table operations
            @param target target device
            @param table_op table operations to send
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
        """
        req = bfruntime_pb2.WriteRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_operation = update.entity.table_operation
        table_operation.table_id = self.info.id_get()
        table_operation.table_operations_type = table_op
        return self.reader_writer_interface._write(req, metadata)

    def handle_get(self, target, key_list, p4_name=None, metadata=None):
        """@brief Get entry handle for specified key.
            @param target target device
            @param key_list List of Keys. In other words, List of list of KeyTuples.
            @param p4_name string containing name of P4 with which to communicate
            optional for subscribed clients, it is mainly used for independent clients
            who want to communicate with a p4 without having to subscribe
            @param metadata : optional metadata to send with write request
            @return handle value of an entry for specified key
        """
        req = bfruntime_pb2.ReadRequest()
        if p4_name:
            req.p4_name = p4_name
        _cpy_target(req, target)
        resp = self.reader_writer_interface._read(self._handle_read_req_make(req, key_list), metadata)
        return self.get_parser._parse_handle_get_response(resp)

    def _set_flags(self, table_entry, flags=None, modify_inc_type=None):
        """Keep support for old mod_inc for the time being. If both flags are set
           raise an error.
        """
        if modify_inc_type != None:
            if modify_inc_type == bfruntime_pb2.TableModIncFlag.MOD_INC_DELETE:
                table_entry.table_flags.mod_del = True
            elif modify_inc_type == bfruntime_pb2.TableModIncFlag.MOD_INC_ADD:
                table_entry.table_flags.mod_del = False
            if flags is not None:
                if flags.get("mod_del") != table_entry.table_flags.mod_del:
                    raise ValueError("Contradicting mod_del flags %s vs %s" \
                        % (flags.get("mod_del"), table_entry.table_flags.mod_del))

        if flags is not None:
            for key,value in flags.items():
                if (key == "from_hw"):
                    table_entry.table_flags.from_hw = value
                elif (key == "key_only"):
                    table_entry.table_flags.key_only = value
                elif (key == "reset_ttl"):
                    table_entry.table_flags.reset_ttl = value
                elif (key == "mod_del"):
                    table_entry.table_flags.mod_del = value

    def _attribute_read_req_make(self, req, attribute_name):
        table_attribute = req.entities.add().table_attribute
        table_attribute.table_id = self.info.id_get()
        if attribute_name == "EntryScope":
            table_attribute.entry_scope.SetInParent()
        elif attribute_name == "IdleTimeout":
            table_attribute.idle_table.SetInParent()
        elif attribute_name == "DynamicHashing":
            table_attribute.dyn_hashing.SetInParent()
        elif attribute_name == "MeterByteCountAdjust":
            table_attribute.byte_count_adj.SetInParent()
        elif attribute_name == "port_status_notif_cb":
            table_attribute.port_status_notify.SetInParent()
        elif attribute_name == "poll_intvl_ms":
            table_attribute.intvl_ms.SetInParent()
        return req

    def _handle_read_req_make(self, req, key_list):
        if key_list is not None and len(key_list) > 0:
            for idx in range(len(key_list)):
                handle = req.entities.add().handle
                handle.table_id = self.info.id_get()
                # Key is the same, for handle and table entry.
                self._set_table_key(handle, list(key_list[idx].field_dict.values()))
        else:
            logger.error("Cannot get handle with empty key list")
            raise ValueError("Cannot get handle with empty key list")
        return req


    def _entry_read_req_make(self, req, key_list,
            flags, required_data, default_entry, handle=None):
        action_name = None
        req_field_list = []
        if required_data is not None:
            action_name = required_data.action_name
            req_field_list = list(required_data.field_dict.values())

        if handle is not None:
            table_entry = req.entities.add().table_entry
            table_entry.table_id = self.info.id_get()
            table_entry.is_default_entry = default_entry
            table_entry.handle_id = handle
            self._set_flags(table_entry, flags)
            self._set_table_data(table_entry, action_name, req_field_list)
        elif key_list is not None and len(key_list) > 0:
            for idx in range(len(key_list)):
                table_entry = req.entities.add().table_entry
                table_entry.table_id = self.info.id_get()
                table_entry.is_default_entry = False
                self._set_flags(table_entry, flags)
                self._set_table_key(table_entry, list(key_list[idx].field_dict.values()))
                self._set_table_data(table_entry, action_name, req_field_list)
        else:
            # read_all / default_entry. If default_entry is true
            # then the default_entry is read, else a read_all operation
            # is performed
            table_entry = req.entities.add().table_entry
            table_entry.table_id = self.info.id_get()
            table_entry.is_default_entry = default_entry
            self._set_flags(table_entry, flags)
            self._set_table_data(table_entry, action_name, req_field_list)
        return req

    def _entry_write_req_make(self, req, key_list, data_list, update_type, modify_inc_type=None, flags=None):
        # if both the lists are present, then check for equality for length of 2
        if key_list is not None and data_list is not None:
            assert(len(key_list) == len(data_list))
        if key_list:
            len_to_iterate = len(key_list)
        else:
            len_to_iterate = len(data_list)

        for idx in range(len_to_iterate):
            update = req.updates.add()
            update.type = update_type
            table_entry = update.entity.table_entry
            table_entry.table_id = self.info.id_get()
            table_entry.is_default_entry = False
            if flags is not None or modify_inc_type is not None:
                self._set_flags(table_entry, flags, modify_inc_type)
            if key_list is not None and key_list[idx] is not None:
                self._set_table_key(table_entry, list(key_list[idx].field_dict.values()))
            if data_list is not None and data_list[idx] is not None:
                self._set_table_data(table_entry, data_list[idx].action_name, list(data_list[idx].field_dict.values()))
        return req

    def _set_table_key(self, table, key_fields):
        """ Sets the key for a bfn::TableEntry object
            @param table : bfn::TableEntry object.
            @param key_fields: List of (name, value, [mask]) tuples.
        """
        if table is None:
            logger.warning("Invalid TableEntry object.")
            return

        for field in key_fields:
            field_id = self.info.key_field_id_get(field.name)
            if field_id is None:
                logger.error("Data key %s not found.", field.name)
            key_field = table.key.fields.add()
            key_field.field_id = field_id
            if field.mask is not None:
                key_field.ternary.value = bytes(field.value)
                key_field.ternary.mask = bytes(field.mask)
            elif field.prefix_len is not None:
                key_field.lpm.value = bytes(field.value)
                key_field.lpm.prefix_len = field.prefix_len
            elif field.low is not None or field.high is not None:
                key_field.range.low = bytes(field.low)
                key_field.range.high = bytes(field.high)
            elif field.is_valid is not None:
                key_field.optional.value = bytes(field.value)
                key_field.optional.is_valid = field.is_valid
            else:
                if isinstance(field.value, str):
                    key_field.exact.value = field.value.encode()
                else:
                    key_field.exact.value = bytes(field.value)

    def _set_table_data(self, table, action, data_fields):
        """ Sets the data for a bfn::TableEntry object
            @param table : bfn::TableEntry object.
            @param ation : Name of the action
            @param data_fields: List of (name, value) DataTuples.
        """
        if action is not None:
            table.data.action_id = self.info.action_id_get(action)

        if data_fields is not None:
            for field in data_fields:
                proto_data_field = table.data.fields.add()
                self._set_table_data_single_field(action, field, proto_data_field)


    def _set_table_data_single_field(self, action, field, proto_data_field):
        proto_data_field.field_id = self.info.data_field_id_get(field.name, action)
        proto_data_field_type = self.info.data_field_type_get(field.name, action)
        if field.val is not None:
            proto_data_field.stream = bytes(field.val)
        elif field.float_val is not None:
            proto_data_field.float_val = field.float_val
        elif field.str_val is not None:
            proto_data_field.str_val = field.str_val
        elif field.bool_val is not None:
            proto_data_field.bool_val = field.bool_val
        elif field.int_arr_val is not None:
            proto_data_field.int_arr_val.val.extend(field.int_arr_val)
        elif field.bool_arr_val is not None:
            proto_data_field.bool_arr_val.val.extend(field.bool_arr_val)
        elif field.str_arr_val is not None:
            proto_data_field.str_arr_val.val.extend(field.str_arr_val)
        elif field.container_arr_val is not None:
            # For container arrays, loop over the list of dicts
            # Each dict is a dict of DataTuples
            for dict_con in field.container_arr_val:
                container = proto_data_field.container_arr_val.container.add()
                for inner_field_name, inner_tuple in dict_con.items():
                    inner_proto_field = container.val.add()
                    self._set_table_data_single_field(None, inner_tuple, inner_proto_field)


class _Learn:
    """@brief Class _Learn (Partially internal). Objects of this class are created
        during BfRtInfo Parsing and do not need
        to be created separately. These objects can be queried from a _BfRtInfo class using a learn_get().
        Contains an interface to interact with a Learn object. In order to
        access metadata associated with it, one needs to access its info object. Apart from metadata
        functions, _Learn object doesn't provide much functionality here since learn_digests are
        received on the streamChannel. Use digest_get on the ClientInterface itself.
    """
    def __init__(self, learn_info, reader_writer_interface):
        self.info = learn_info
        self.reader_writer_interface = reader_writer_interface
        self.get_parser = _GetParser(self)

    def make_data(self, data_field_list_in=[], *unused):
        """@brief Create a _Data object using a list of DataTuple
            @param data_field_list_in List of DataTuple objects.
        """
        return _Data(self, data_field_list_in, None, False)

    def make_data_list(self, digest):
        """@brief Create a list of _Data objects using a learn_digest msg
            @param digest learn digest
        """
        data_list = []
        for data_entry in digest.data:
            data_list.append(self.get_parser._parse_data(data_entry, False))
        return data_list

class _BfRtInfo:
    """@brief Class _BfRtInfo (Partially internal). An object of this class is created when bfrt_info_get is called on the
        ClientInterface. It internally parses bf-rt.json and keeps _Table and _Learn objects inside which can
        be queried using table_get or learn_get. It wraps over class BfRtInfoParser and keeps an object
        of it internally
    """
    def __init__(self, p4_name, p4_json_data, non_p4_json_data, reader_writer_interface):

        self.parsed_info = info_parse.BfRtInfoParser(p4_json_data, non_p4_json_data)
        self.p4_name = p4_name
        self.table_dict = {}
        self.learn_dict = {}

        self.table_id_dict = {}
        self.learn_id_dict = {}

        self.reader_writer_interface = reader_writer_interface

        _BfRtInfo._insert_objs_in_dict(
                self.parsed_info.table_info_dict_get(),
                self.table_dict,
                self.reader_writer_interface,
                True)
        _BfRtInfo._insert_objs_in_dict(
                self.parsed_info.learn_info_dict_get(),
                self.learn_dict,
                self.reader_writer_interface,
                False)
        # Create a map for tables and learn objects from id<->object too
        _BfRtInfo._create_id_map(self.table_dict, self.table_id_dict)
        _BfRtInfo._create_id_map(self.learn_dict, self.learn_id_dict)


    def __str__(self):
        return self.parsed_info.__str__()

    def key_from_idletime_notification(self, idletimeout_notification_message):
        entry = idletimeout_notification_message.table_entry
        table_id = entry.table_id
        found_table = None
        for name, table in self.table_dict.items():
            if table.info.id_get() == table_id:
                found_table = table
        if found_table is None:
            raise RuntimeError("%d table ID not found in bfrt info of %s" %(table_id, self.p4_name))
        return found_table.get_parser._parse_key(entry.key)

    @staticmethod
    def _create_id_map(obj_map, id_map):
        for obj_name, obj in obj_map.items():
            obj_id = obj.info.id_get()
            # The object can be present in the obj_map multiple times
            # for different names. We want to ignore such a case.
            # Error only for a strange case where the same ID is already present
            # but we got a different item for it.
            if obj_id in id_map:
                if obj != id_map[obj_id]:
                    raise RuntimeError("%s found is different than obj %s" %(obj.info.name_get(), id_map[obj_id].info.name_get()))
                else:
                    continue
            id_map[obj_id] = obj

    @staticmethod
    def _insert_objs_in_dict(info_dict, main_obj_dict, rw_interface, is_table=True):
        """@brief Add objects to the main dictionary. Same object
            can be referenced by many names depending upon uniqueness
        """
        names_to_remove = set()
        for obj_name, obj_info in info_dict.items():
            # Make all possible names for the object and try to insert them
            # in the dictionary
            obj = None
            if is_table:
                obj = _Table(obj_info, rw_interface)
            else:
                obj = _Learn(obj_info, rw_interface)
            possible_name_list = info_parse._generate_unique_names(obj_name)
            for prospective_name in possible_name_list:
                if prospective_name in main_obj_dict:
                    names_to_remove.add(prospective_name)
                else:
                    main_obj_dict[prospective_name] = obj
        for name in names_to_remove:
            main_obj_dict.pop(name, None)

    def p4_name_get(self):
        """@brief Get p4_name associated with this _BfRtInfo
        object
        """
        return self.p4_name

    def table_get(self, name):
        """@brief Get a Table object from name
            @param name Table-name
            @return Table object
        """
        return self.table_dict[name]

    def learn_get(self, name):
        """@brief Get a Learn object from name
            @param name Learn-object-name
            @return Learn object
        """
        return self.learn_dict[name]

    def table_from_id_get(self, obj_id):
        """@brief Get a Table object from ID
            @param name bfrt_info ID
            @return Table object
        """
        return self.table_id_dict[obj_id]

    def learn_from_id_get(self, obj_id):
        """@brief Get a Learn object from ID
            @param name bfrt_info ID
            @return Learn object
        """
        return self.learn_id_dict[obj_id]

