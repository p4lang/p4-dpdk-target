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

import sys
import os
import logging
from typing import List

STREAM_HANDLER = logging.StreamHandler(stream=sys.stdout)

def initialize_logger(package_name) -> logging.Logger:
    STREAM_HANDLER.setLevel(logging.CRITICAL)
    logger = logging.getLogger(package_name)
    logging.basicConfig(level=logging.DEBUG, handlers=[STREAM_HANDLER])
    return logger
