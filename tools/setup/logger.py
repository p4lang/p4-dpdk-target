# SPDX-FileCopyrightText: 2021 Intel Corporation
# Copyright (C) 2021 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

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
