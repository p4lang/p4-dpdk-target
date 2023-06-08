#!/usr/bin/env python3

import argparse
import logging
import string
from pathlib import Path
from subprocess import run

import config
import util


def main():
    logging.basicConfig(level=logging.INFO)

    parser = argparse.ArgumentParser()
    parser.add_argument("test_dir", type=Path)
    args = parser.parse_args()

    run_bf_switchd(args.test_dir)


def run_bf_switchd(test_dir, in_background=False):
    util.assert_exists(test_dir)

    # Generate bf_switchd conf file
    conf_template_path = config.THIS_DIR / "template" / config.SUITE_BF_SWITCHD_CONF
    conf_template = string.Template(conf_template_path.read_text())
    # These paths have to be absolute,
    # or they will be interpreted as relative to $SDE_INSTALL.
    test_dir = test_dir.resolve()
    conf = conf_template.substitute(
        bfrt_config=test_dir / config.SUITE_BFRT,
        context=test_dir / config.SUITE_CONTEXT,
        config=test_dir / config.SUITE_SPEC,
    )
    conf_path = test_dir / config.SUITE_BF_SWITCHD_CONF
    conf_path.write_text(conf)

    # Configure hugepages for DPDK
    sde_env = util.get_sde_env()
    sde_install = sde_env["SDE_INSTALL"]
    run(f"{sde_install}/bin/dpdk-hugepages.py -p 2M --setup 32M", shell=True)
    run(f"{sde_install}/bin/dpdk-hugepages.py -s", shell=True)

    # Run the command
    bin_name = "bf_switchd"
    bin_path, log_dir, sde_env = util.bf_prepare(test_dir, bin_name)
    cmd = [bin_path]
    cmd += ["--install-dir", sde_env["SDE_INSTALL"]]
    cmd += ["--conf-file", conf_path.resolve()]
    return util.bf_run(cmd, log_dir, sde_env, in_background, with_sudo=True)


if __name__ == "__main__":
    main()
