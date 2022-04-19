#!/bin/bash
# Start running PTF tests associated with a P4 program
set -x
function print_help() {
  echo "USAGE: $(basename ""$0"") {-p <...> | -t <...>} [OPTIONS -- PTF_OPTIONS]"
  echo "Options for running PTF tests:"
  echo "  -p <p4_program_name>"
  echo "    Run PTF tests associated with P4 program"
  echo "  -t TEST_DIR"
  echo "    TEST_DIR contains test cases executed by PTF."
  echo "  -f PORTINFO_FILE"
  echo "    Read port to veth mapping information from PORTINFO_FILE"
  echo "  -s TEST_SUITE"
  echo "    Name of the test suite to execute passed to PTF"
  echo "  -c TEST_CASE"
  echo "    Name of the test case to execute passed to PTF"
  echo "  -m MAX_PORTS"
  echo "    --max-ports that is passed to PTF"
  echo "  --reboot <TYPE>"
  echo "    Run reboot sequence [hitless/fastreconfig] for all the applicable ptf test cases"
  echo "  --target <TARGET>"
  echo "    Target (asic-model or hw)"
  echo "  --no-veth"
  echo "    Skip veth setup and special CPU port setup"
  echo "  --config-file"
  echo "    PTF config-file"
  echo "  --num-pipes NUM_PIPES"
  echo "    Set num of pipes to use for test"
  echo "  --drv-test-info <file>"
  echo "    Specify the driver combination test config file"
  echo "  --seed <number>"
  echo "    Specify the driver combination test random seed"
  echo "  --ip <target switch IP address>"
  echo "    Target switch's IP address, localhost by default"
  echo "  --thrift-server <thrift_server_address>"
  echo "    Depreciated, use --ip"
  echo "  --setup"
  echo "    Run test setup only"
  echo "  --cleanup"
  echo "    Run test cleanup only"
  echo "  --traffic-gen <traffic_generator>"
  echo "    Traffic Generator (ixia, scapy)"
  echo "  --socket-recv-size <socket bytes size>"
  echo "    socket buffer size for ptf scapy verification "
  echo "  --no-status-srv"
  echo "    Do not query bf_switchd's status server"
  echo "  --failfast"
  echo "    Fail and exit on first failure"
  echo "  --status-port <port number>"
  echo "    Specify bf_switchd's status server port number; the default is 7777"
  echo "  --test-params <ptf_test_params>"
  echo "    PTF test params as a string, e.g. target='hw';"
  echo "  --with-p4c <compiler version>"
  echo "    P4C compiler version, e.g. v6"
  echo "  --gen-xml-output <gen_xml_output>"
  echo "    Specify this flag to generate xml output for tests"
  echo "  --db-prefix"
  echo "    Database prefix to pass to PTF"
  echo "  --p4info"
  echo "    Path to P4Info Protobuf text file for P4Runtime tests"
  echo "  --default-timeout"
  echo "    Timeout in seconds for most operations"
  echo "  --default-negative-timeout"
  echo "    Timeout in seconds for negative checks"
  echo "  --profile"
  echo "    Enable Python profiling"
  echo "  -h"
  echo "    Print this message"
  exit 0
}

trap 'exit' ERR

if [ -z $WORKSPACE ]; then
  WORKSPACE=`pwd`
fi
echo "Using workspace ${WORKSPACE}"

export BF_SDE_ROOT=${WORKSPACE}
if [ -z $BF_SDE_INSTALL_DIR ]; then
export BF_SDE_INSTALL_DIR=${WORKSPACE}/install
fi

opts=`getopt -o p:t:f:s:c:m:h --long reboot: --long config-file: --long target: --long num-pipes: --long drv-test-info: --long failfast --long seed: --long no-veth --long thrift-server: --long setup --long cleanup --long traffic-gen: --long socket-recv-size: --long no-status-srv --long status-port: --long ip: --long test-params: --long port-mode: --long with-p4c: --long gen-xml-output --long db-prefix: --long p4info: --long default-negative-timeout: --long default-timeout: --long profile -- "$@"`

if [ $? != 0 ]; then
  exit 1
fi
eval set -- "$opts"

# P4 program name
P4_NAME=""
# json file specifying model port to veth mapping info
PORTINFO=None
MAX_PORTS=17
NUM_PIPES=4
CONFIG_FILE='cfg'
CPUPORT=64
CPUVETH=251
TARGET_IP='localhost'
REBOOT_TYPE='None'
NO_VETH=false
HELP=false
SETUP=""
CLEANUP=""
TARGET="asic-model"
TRAFFIC_GEN="None"
SOCKET_RECV_SIZE="10240"
SKIP_STATUS_SRV=false
FAILFAST=""
TEST_PARAMS=""
PORTMODE=""
COMPILER_VERSION="v5"
GEN_XML_OUTPUT=0
DB_PREFIX=""
P4INFO_PATH=""
PROFILE=""
DEFAULT_TIMEOUT=""
DEFAULT_NEGATIVE_TIMEOUT=""
PTF_BINARY=""

while true; do
    case "$1" in
      -p) P4_NAME=$2; shift 2;;
      -t) TEST_DIR="$2"; shift 2;;
      -s) TEST_SUITE="$2"; shift 2;;
      -c) TEST_CASE="$2"; shift 2;;
      -f) PORTINFO=$2; shift 2;;
      -m) MAX_PORTS=$2; shift 2;;
      -h) HELP=true; shift 1;;
      --reboot) REBOOT_TYPE=$2; shift 2;;
      --no-veth) NO_VETH=true; shift 1;;
      --config-file) CONFIG_FILE=$2; shift 2;;
      --num-pipes) NUM_PIPES=$2; shift 2;;
      --drv-test-info) DRV_TEST_INFO="--drivers-test-info $2"; shift 2;;
      --seed) DRV_TEST_SEED="--seed $2"; shift 2;;
      --target) TARGET=$2; shift 2;;
      --thrift-server) TARGET_IP=$2; shift 2;;
      --ip) TARGET_IP=$2; shift 2;;
      --traffic-gen) TRAFFIC_GEN=$2; shift 2;;
      --socket-recv-size) SOCKET_RECV_SIZE=$2; shift 2;;
      --failfast) FAILFAST="--failfast"; shift 1;;
      --setup) SETUP="--setup"; shift 1;;
      --cleanup) CLEANUP="--cleanup"; shift 1;;
      --status-port) STS_PORT=$2; shift 2;;
      --no-status-srv) SKIP_STATUS_SRV=true; shift 1;;
      --test-params) TEST_PARAMS=$2; shift 2;;
      --port-mode) PORTMODE="--port-mode $2"; shift 2;;
      --with-p4c) COMPILER_VERSION=$2; shift 2;;
      --gen-xml-output) GEN_XML_OUTPUT=1; shift 1;;
      --db-prefix) DB_PREFIX="--db-prefix $2"; shift 2;;
      --p4info) P4INFO_PATH=$2; shift 2;;
      --default-timeout) DEFAULT_TIMEOUT="--default-timeout $2"; shift 2;;
      --default-negative-timeout) DEFAULT_NEGATIVE_TIMEOUT="--default-negative-timeout $2"; shift 2;;
      --profile) PROFILE="--profile"; shift 1;;
      --) shift; break;;
    esac
done

if [ $HELP = true ] || ( [ -z $P4_NAME ] && [ -z $TEST_DIR ] ); then
  print_help
fi

if [ $NO_VETH = true ]; then
  CPUPORT=None
  CPUVETH=None
fi

if [ -z ${TEST_DIR} ]; then
  if [[ $P4_NAME == "switch" ]]; then
    TEST_DIR=$BF_SDE_ROOT/submodules/bf-switch/ptf/api
  else
    TEST_DIR=$BF_SDE_ROOT/submodules/p4-tests/ptf-tests/$P4_NAME
    if [ ! -d $TEST_DIR ]; then
      TEST_DIR=$BF_SDE_ROOT/submodules/p4-tests/p4_16_programs/$P4_NAME
    fi
    if [ ! -d $TEST_DIR ]; then
      TEST_DIR=$BF_SDE_ROOT/submodules/p4-tests/internal_p4_16/$P4_NAME
    fi
    if [ ! -d $TEST_DIR ]; then
      TEST_DIR=$BF_SDE_ROOT/submodules/p4-tests/internal_p4_14/$P4_NAME
    fi
  fi
fi

[ -d "$TEST_DIR" ] || exit "Test directory $TEST_DIR directory does not exist"

echo "Using TEST_DIR ${TEST_DIR}"

if [[ $PORTINFO != None ]]; then
  CPUPORT=None
  CPUVETH=None
fi

PYTHON_VER=`python --version 2>&1 | awk {'print $2'} | awk -F"." {'print $1"."$2'}`

export PATH=$BF_SDE_INSTALL_DIR/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/lib:$BF_SDE_INSTALL_DIR/lib:$LD_LIBRARY_PATH
PYTHONPATH=$BF_SDE_INSTALL_DIR/lib/python$PYTHON_VER/site-packages::$PYTHONPATH

# Environment variable PKTPY allows switching between PTF with scapy or PTF with bf-pktpy.
if [ "$PKTPY" = "True" ]; then
  echo "Using bf-pktpy as a packet manipulation framework."
  PTF_BINARY="--ptf bf-ptf"
  PYTHONPATH=$BF_SDE_INSTALL_DIR/lib/python$PYTHON_VER/site-packages/bf-ptf:$PYTHONPATH
fi

echo "Using PATH ${PATH}"
echo "Using LD_LIBRARY_PATH ${LD_LIBRARY_PATH}"
echo "Using PYTHONPATH ${PYTHONPATH}"

echo "Reboot type is $REBOOT_TYPE"

# Setup veth interfaces
if [ $NO_VETH = false ]; then
  echo "Setting up veth interfaces"
  sudo env "PATH=$PATH" $BF_SDE_INSTALL_DIR/bin/veth_setup.sh
fi
echo "Target is $TARGET"

# Check in with bf_switchd's status server to make sure it is ready
STS_PORT_STR="--port 7777"
if [ "$STS_PORT" != "" ]; then
  STS_PORT_STR="--port $STS_PORT"
fi
STS_HOST_STR="--host $TARGET_IP"
if [ $SKIP_STATUS_SRV = false ]; then
  python $BF_SDE_INSTALL_DIR/lib/python$PYTHON_VER/site-packages/p4testutils/bf_switchd_dev_status.py \
    $STS_HOST_STR $STS_PORT_STR
fi

TEST_PARAMS_STR=""
if [ "$TEST_PARAMS" != "" ]; then
    TEST_PARAMS_STR="--test-params $TEST_PARAMS"
fi

#Run PTF tests
sudo env -u http_proxy -u socks_proxy "PATH=$PATH" \
	"PYTHONPATH=$PYTHONPATH" "GEN_XML_OUTPUT=$GEN_XML_OUTPUT" \
	"PKTPY=$PKTPY" python \
    $BF_SDE_INSTALL_DIR/lib/python$PYTHON_VER/site-packages/p4testutils/run_ptf_tests.py \
    --test-dir $TEST_DIR \
    $TEST_SUITE \
    $TEST_CASE \
    --target $TARGET \
    --port-info $PORTINFO \
    $DEFAULT_TIMEOUT \
    $PORTMODE \
    #-cpu-port $CPUPORT \
    #--cpu-veth $CPUVETH \
    #--max-ports $MAX_PORTS \
    #--num-pipes $NUM_PIPES \
    #--config-file $CONFIG_FILE \
    #--thrift-server $TARGET_IP \
    #--grpc-server $TARGET_IP \
    #--traffic-gen $TRAFFIC_GEN \
    #--socket-recv-size $SOCKET_RECV_SIZE \
    #--p4c $COMPILER_VERSION \
    #--reboot $REBOOT_TYPE \
    $DRV_TEST_INFO \
    $PTF_BINARY \
    $DEFAULT_NEGATIVE_TIMEOUT \
    $PROFILE \
    $DRV_TEST_SEED $FAILFAST $SETUP $CLEANUP $TEST_PARAMS_STR $DB_PREFIX $@
