# Copyright (c) Meta Platforms, Inc. and affiliates.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

from contextlib import contextmanager
from typing import Generator

import torch


@contextmanager
def tmp_rng_seed(device: torch.device, seed: int = 0) -> Generator:
    """Sets a temporary manual RNG seed.

    The RNG is reset to its original state once the block is exited.
    """
    if device.type == "cuda":
        devices = [device]
    else:
        devices = []

    with torch.random.fork_rng(devices):
        torch.manual_seed(seed)

        yield