#!/bin/bash

if [ -z $SDE ]; then
    echo "Set env variable 'SDE' for SDE path."
    exit 1
fi

if [ -z $SDE_INSTALL ]; then
    echo "Set env variable 'SDE_INSTALL' for SDE path."
    exit 1
fi

if [ ! -d $SDE ]; then
    echo "SDE dir configured is not present in the system.
          Please check and configure."
    exit 1
fi

if [ ! -d $SDE_INSTALL ]; then
    echo "SDE_INSTALL dir configured is not present in the system.
         Please check and configure."
    exit 1
fi

RUN_P4_TEST=$SDE_INSTALL/bin/run_p4_tests.sh
DEFAULT_TIMEOUT=180
TIME=`date "+%Y.%m.%d-%H.%M.%S"`
LOG_DIR=$SDE/ptf-results
mkdir -p $LOG_DIR
LOG_FILE=$LOG_DIR/ptf-suite-$TIME.log

test_suite1()
{
    echo "****************************** Starting Test : Simple_l2l3_lpm \
******************************"

    echo "****************************** Port Addition Config Start \
***********************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	    	 -p $1 -t $2 --no-veth -s switchdportadd
    echo "****************************** Port Addition Config End \
*************************************"

    echo "****************************** Enable Pipline Config Start \
**********************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth -s switchdenablepipeline
    echo "****************************** Enable Pipeline Config End \
***********************************"

    echo "****************************** Add Table entries Config Start \
*******************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth -s switchdaddtableentry
    echo "****************************** Add Table entries Config End \
*********************************"

    echo "****************************** Verifiation of packet Start \
**********************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth \
		 -f $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd/port_info.json \
		 -s switchdverifytraffic
    echo "****************************** Verification of packet End \
***********************************"

    echo "****************************** Cleanup Config Start \
*****************************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth -s switchdcleantable
    echo "****************************** Cleanup Config End \
*******************************************"
    echo "****************************** End of Simple_l2l3_lpm Test \
**********************************"
}

test_suites()
{
    test_suite1 simple_l2l3_lpm $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd
}

test_results()
{
    result1=`grep -c "PTF test result: Success" $LOG_FILE`
    result2=`grep -c "PTF test result: Failure" $LOG_FILE`
    result3=`grep -c "PTF test result: Unknown" $LOG_FILE`
    tests=`grep -c "Starting Test" $LOG_FILE`
    echo "******************************PTF RESULT****************************"
    echo "Configs with status Pass: $result1"
    echo "Configs with status Failed: $result2"
    echo "Configs with status Unknown: $result3"
    echo "Total tests executed: $tests"
    echo "Log file for PTF run: $LOG_FILE"
    echo "******************************PTF RESULT****************************"
}

test_suites 2>&1 | tee $LOG_FILE
test_results
