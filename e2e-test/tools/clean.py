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

    clean(args.test_dir)


def clean(test_dir):
    util.assert_exists(test_dir)

    for path in config.SUITE_PATH_GENERATED:
        util.remove_file_or_dir(test_dir / path)


if __name__ == "__main__":
    main()
