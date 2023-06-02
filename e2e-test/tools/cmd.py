#!/usr/bin/env python3

import argparse
import logging
import string
from pathlib import Path
from subprocess import run

import util

THIS_DIR = Path(__file__).resolve().parent

# Test suite path conventions

SUITE_MAIN_P4 = "main.p4"
SUITE_BF_SWITCHD_CONF = "bf_switchd_conf.json"
SUITE_BFSHELL_CMDS = "bfshell_cmds.py"

SUITE_P4C_GEN = "p4c_gen"
SUITE_P4INFO = SUITE_P4C_GEN + "/p4info.txt"
SUITE_BFRT = SUITE_P4C_GEN + "/bfrt.json"
SUITE_TDI = SUITE_P4C_GEN + "/tdi.json"
SUITE_CONTEXT = SUITE_P4C_GEN + "/context.json"
SUITE_SPEC = SUITE_P4C_GEN + "/config.spec"

SUITE_LOG = "log"

SUITE_PATH_GENERATED = [SUITE_BF_SWITCHD_CONF, SUITE_P4C_GEN, SUITE_LOG]


# Main entry point
def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(required=True)

    add_parser_for_compile(subparsers)
    add_parser_for_bf_switchd(subparsers)
    add_parser_for_bfshell(subparsers)
    add_parser_for_clean(subparsers)

    args = parser.parse_args()
    args.func(args)


# The compile command


def add_parser_for_compile(subparsers):
    parser = subparsers.add_parser("compile")
    parser.add_argument("test_dir", type=Path)
    parser.set_defaults(func=compile)


def compile(args):
    test_dir = args.test_dir

    # Make sure test dir exists
    assert test_dir.exists(), f"{test_dir} doesn't exist"

    # Prepare compiler output dir
    output_dir = test_dir / SUITE_P4C_GEN
    if output_dir.exists():
        logging.info(f"Removing existing compiler output dir {output_dir}")
        util.remove_file_or_dir(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    # Compile P4 source
    arch = test_dir.parent.stem
    test_dir_prefix = test_dir.as_posix() + "/"
    p4_source = test_dir_prefix + SUITE_MAIN_P4
    cmd = ["p4c-dpdk"]
    cmd += ["--arch", arch]
    cmd += ["--p4runtime-files", test_dir_prefix + SUITE_P4INFO]
    cmd += ["--bf-rt-schema", test_dir_prefix + SUITE_BFRT]
    cmd += ["--tdi", test_dir_prefix + SUITE_TDI]
    cmd += ["--context", test_dir_prefix + SUITE_CONTEXT]
    cmd += ["-o", test_dir_prefix + SUITE_SPEC]
    cmd += [p4_source]
    logging.info(f"Compiling {p4_source}")
    util.log_cmd(cmd)
    run(cmd, check=True)
    logging.info("Compiling succeeds")


# The bf_switchd command


def add_parser_for_bf_switchd(subparsers):
    parser = subparsers.add_parser("bf_switchd")
    parser.add_argument("test_dir", type=Path)
    parser.set_defaults(func=bf_switchd)


def bf_switchd(args):
    test_dir = args.test_dir

    # Make sure test dir exists
    assert test_dir.exists(), f"{test_dir} doesn't exist"

    # Generate bf_switchd conf file
    conf_template_path = THIS_DIR / "template" / SUITE_BF_SWITCHD_CONF
    conf_template = string.Template(conf_template_path.read_text())
    # These paths have to be absolute,
    # or they will be interpreted as relative to $SDE_INSTALL
    test_dir_prefix = test_dir.resolve().as_posix() + "/"
    conf = conf_template.substitute(
        bfrt_config=test_dir_prefix + SUITE_BFRT,
        context=test_dir_prefix + SUITE_CONTEXT,
        config=test_dir_prefix + SUITE_SPEC,
    )
    conf_path = test_dir / SUITE_BF_SWITCHD_CONF
    conf_path.write_text(conf)

    # Configure hugepages for DPDK
    run("dpdk-hugepages.py -p 1G --setup 2G", shell=True, check=True)
    run("dpdk-hugepages.py -s", shell=True, check=True)

    # Run bf_switchd
    # Have to switch to the SDE environment.
    sde_env = util.get_sde_env()
    sde_install = sde_env["SDE_INSTALL"]
    bin = f"{sde_install}/bin/bf_switchd"
    assert Path(bin).exists(), f"Required binary {bin} doesn't exist"
    log_dir = test_dir / "log/bf_switchd/"
    log_dir.mkdir(parents=True, exist_ok=True)
    cmd = [bin]
    cmd += ["--install-dir", sde_env["SDE_INSTALL"]]
    cmd += ["--conf-file", conf_path.resolve().as_posix()]
    util.log_cmd(cmd)
    run(cmd, cwd=log_dir, env=sde_env, check=True)


# The bfshell command


def add_parser_for_bfshell(subparsers):
    parser = subparsers.add_parser("bfshell")
    parser.add_argument("test_dir", type=Path)
    parser.set_defaults(func=bfshell)


def bfshell(args):
    test_dir = args.test_dir

    # Make sure test dir exists
    assert test_dir.exists(), f"{test_dir} doesn't exist"

    # Run bfshell
    # Have to switch to the SDE environment.
    sde_env = util.get_sde_env()
    sde_install = sde_env["SDE_INSTALL"]
    bin = f"{sde_install}/bin/bfshell"
    assert Path(bin).exists(), f"Required binary {bin} doesn't exist"
    cmd = [bin]
    cmd += ["-f", (test_dir / SUITE_BFSHELL_CMDS).resolve().as_posix()]
    run(cmd, env=sde_env, check=True)


# The clean command


def add_parser_for_clean(subparsers):
    parser = subparsers.add_parser("clean")
    parser.add_argument("test_dir")
    parser.set_defaults(func=clean)


def clean(args):
    test_dir = Path(args.test_dir)

    for path in SUITE_PATH_GENERATED:
        util.remove_file_or_dir(test_dir / path)


# Start

if __name__ == "__main__":
    main()
