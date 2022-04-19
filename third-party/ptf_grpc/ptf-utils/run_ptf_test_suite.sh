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

export LD_LIBRARY_PATH=$SDE_INSTALL/lib/:$SDE_INSTALL/lib/x86_64-linux-gnu:$SDE_INSTALL/lib64
export PKG_CONFIG_PATH=$SDE_INSTALL/lib/x86_64-linux-gnu/pkgconfig:$SDE_INSTALL/lib64/pkgconfig

RUN_P4_TEST=$SDE_INSTALL/bin/run_p4_tests.sh
DEFAULT_TIMEOUT=180
TIME=`date "+%Y.%m.%d-%H.%M.%S"`
LOG_DIR=$SDE/ptf-results
mkdir -p $LOG_DIR
LOG_FILE=$LOG_DIR/ptf-suite-$TIME.log
bf_switchd_app=$SDE_INSTALL/bin/bf_switchd

generate_p4_artifacts()
{
   $SDE_INSTALL/bin/gen_p4_artifacts.sh
}

#this func will kill bf_switchd
kill_bf_switchd()
{
   echo "****************************** Killing bf_switchd \
******************************"
   ps -aux | grep bf_switchd | grep -v grep | awk '{print $2}' | xargs sudo kill -9 || true
   sleep 1
}

#this func will start bf_switchd with conf file passed as arg
start_bf_switchd()
{
   echo "****************************** Starting bf_switchd \
******************************"
   $bf_switchd_app --install-dir $SDE_INSTALL --conf-file $1 \
	   --init-mode=cold --status-port 7777 --background &
   sleep 2

}

#Common configuration which each test executes.
common_configs()
{
   echo "****************************** Starting Common Configs \
**************************************"

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

}

#this is simple_l2l3_lpm test suite
test_suite1()
{
    echo "****************************** Starting Test : Simple_l2l3_lpm \
******************************"

    echo "****************************** LPM:Add Table entries Config Start \
*************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth -s switchdaddtableentry
    echo "****************************** LPM:Add Table entries Config End \
*****************************"

    echo "****************************** LPM:Verifiation of packet Start \
*******************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth \
		 -f $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd/port_info.json \
		 -s switchdverifytraffic
    echo "****************************** LPM:Verification of packet End \
*******************************"

    echo "****************************** LPM:Cleanup Config Start \
*************************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth -s switchdcleantable
    echo "****************************** LPM:Cleanup Config End \
***************************************"
    echo "****************************** End of Simple_l2l3_lpm Test \
**********************************"
}

#this is simple_l2l3_wcm test suite
test_suite2()
{
    echo "****************************** Starting Test : Simple_l2l3_wcm \
******************************"

    echo "****************************** WCM:Add Table entries Config Start \
*******************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth -s switchdaddwcmtableentry
    echo "****************************** WCM:Add Table entries Config End \
*********************************"

    echo "****************************** WCM:Verifiation of packet Start \
**********************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth \
		 -f $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd/port_info.json \
		 -s switchdverifywcmtraffic
    echo "****************************** WCM:Verification of packet End \
***********************************"

    echo "****************************** WCM:Cleanup Config Start \
*****************************************"
    $RUN_P4_TEST --default-timeout $DEFAULT_TIMEOUT \
	         -p $1 -t $2 --no-veth -s switchdcleanwcmtable
    echo "****************************** WCM:Cleanup Config End \
*******************************************"
    echo "****************************** End of Simple_l2l3_wcm Test \
**********************************"
}

#this func start/kill bf_switchd and executes the test cases
test_suites()
{
    #if You want to run the Tests with generatig artifacts uncomment below line.
    #generate_p4_artifacts
    kill_bf_switchd
    start_bf_switchd $SDE_INSTALL/sample_app/simple_l2l3_lpm/simple_l2l3_lpm.conf
    common_configs simple_l2l3_lpm \
	    $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd
    test_suite1 simple_l2l3_lpm \
	    $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd
    kill_bf_switchd
    start_bf_switchd $SDE_INSTALL/sample_app/simple_l2l3_wcm/simple_l2l3_wcm.conf
    common_configs simple_l2l3_wcm \
	    $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd
    test_suite2 simple_l2l3_wcm \
	    $SDE/driver/third-party/ptf_grpc/ptf-tests/switchd
    kill_bf_switchd
}

#this func prints the results of the test suites and configuration
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
