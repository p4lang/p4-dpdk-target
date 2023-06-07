#!/usr/bin/env python3

import argparse
import atexit
import logging
import time
from pathlib import Path
from subprocess import run

import config
import util
from clean import clean
from compile import compile
from run_bf_switchd import run_bf_switchd
from run_bfshell import run_bfshell


def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser()
    parser.add_argument("test_dir", type=Path)
    args = parser.parse_args()

    test(args.test_dir)


def test(test_dir):
    util.assert_exists(test_dir)

    clean(test_dir)
    compile(test_dir)

    # Run bf_switchd in background
    # Wait until bfshell is up
    p, stdout = run_bf_switchd(test_dir, in_background=True)
    atexit.register(p.kill)
    while not stdout.read_text().endswith("bfshell> "):
        time.sleep(1)

    # Run some bfshell commands
    # Wait until all commands are finished
    p, stdout = run_bfshell(test_dir, in_background=True)
    atexit.register(p.kill)
    while "finished" not in stdout.read_text():
        time.sleep(1)

    # Run some shell commands
    cmd = ["bash", test_dir / config.SUITE_CMDS_SHELL]
    run(cmd, check=True)

    # Run the testing script
    cmd = ["python3", test_dir / config.SUITE_TEST]
    run(cmd)


if __name__ == "__main__":
    main()
