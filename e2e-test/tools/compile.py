#!/usr/bin/env python3

import argparse
import logging
from pathlib import Path
from subprocess import run

import config
import util


def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser()
    parser.add_argument("test_dir", type=Path)
    args = parser.parse_args()

    compile(args.test_dir)


def compile(test_dir):
    util.assert_exists(test_dir)

    # Prepare compiler output dir
    output_dir = test_dir / config.SUITE_P4C_GEN
    if output_dir.exists():
        logging.info(f"Removing existing compiler output dir {output_dir}")
        util.remove_file_or_dir(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Compile P4 source
    arch = test_dir.parent.stem
    p4_source = test_dir / config.SUITE_MAIN_P4
    cmd = ["p4c-dpdk"]
    cmd += ["--arch", arch]
    cmd += ["--p4runtime-files", test_dir / config.SUITE_P4INFO]
    cmd += ["--bf-rt-schema", test_dir / config.SUITE_BFRT]
    cmd += ["--tdi", test_dir / config.SUITE_TDI]
    cmd += ["--context", test_dir / config.SUITE_CONTEXT]
    cmd += ["-o", test_dir / config.SUITE_SPEC]
    cmd += [p4_source]
    util.log_cmd(cmd)
    run(cmd, check=True)


if __name__ == "__main__":
    main()
