#!/usr/bin/env bash
# https://github.com/p4lang/p4-dpdk-target#running-reference-app
set -e

# Per test settings
TEST_NAME=pna/simple_l2_forwarding
TEST_DIR="$PWD"

# Make sure SDE environment variables are set
[ -z "$SDE" ] && echo "Environment variable SDE not set" && exit 1
[ -z "$SDE_INSTALL" ] && echo "Environment variable SDE_INSTALL not set" && exit 1
echo "Using SDE: $SDE"
echo "Using SDE_INSTALL: $SDE_INSTALL"

# Configure hugepages for DPDK
# This has to be done before switching to the SDE environment.
dpdk-hugepages.py -p 1G --setup 2G 
dpdk-hugepages.py -s

# Switch to the SDE environment
export LD_LIBRARY_PATH="$SDE_INSTALL/lib:$SDE_INSTALL/lib64:$SDE_INSTALL/lib/x86_64-linux-gnu"
export PYTHONPATH="$SDE_INSTALL/lib/python3.10/:$SDE_INSTALL/lib/python3.10/lib-dynload:$SDE_INSTALL/lib/python3.10/site-packages"
export PYTHONHOME="$SDE_INSTALL/lib/python3.10"

# All files referred to in the bf_switchd conf file (pipe/config.spec) have to be under $SDE_INSTALL,
# so let's copy them over.
SDE_CONF_DIR="$SDE_INSTALL/share/e2e-test/$TEST_NAME"
rm -rf "$SDE_CONF_DIR"
mkdir -p "$SDE_CONF_DIR"
cp -r "$TEST_DIR/pipe" "$SDE_CONF_DIR/pipe"

# Run bf_switchd in the log directory to collect all log files there
CONF_FILE="$TEST_DIR/bf_switchd_conf.json"
LOG_DIR="$TEST_DIR/log/bf_switchd"
mkdir -p "$LOG_DIR"
cd "$LOG_DIR"
"$SDE_INSTALL/bin/bf_switchd" \
    --install-dir "$SDE_INSTALL" \
    --conf-file "$CONF_FILE"
