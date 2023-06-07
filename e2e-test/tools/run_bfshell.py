#!/usr/bin/env python3

import argparse
import logging
from pathlib import Path

import config
import util


def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser()
    parser.add_argument("test_dir", type=Path)
    args = parser.parse_args()

    run_bfshell(args.test_dir)


def run_bfshell(test_dir, in_background=False):
    util.assert_exists(test_dir)

    # Run the command
    bin_name = "bfshell"
    bin_path, log_dir, sde_env = util.bf_prepare(test_dir, bin_name)
    cmd = [bin_path]
    cmd += ["-f", (test_dir / config.SUITE_CMDS_BFSHELL).resolve()]
    return util.bf_run(cmd, log_dir, sde_env, in_background)


if __name__ == "__main__":
    main()
