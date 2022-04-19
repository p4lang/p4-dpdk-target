#!/bin/bash

TIME=`date "+%Y.%m.%d-%H.%M.%S"`
LOG_DIR=$SDE/compilation-logs
mkdir -p $LOG_DIR
LOG_FILE=$LOG_DIR/compilation-$TIME.log

#Cloning the P4C Repo

if [ -z $SDE ]; then
	echo "Please export SDE & SDE_INSTALL first"
	exit 1
fi
cd $SDE

clone_dir=p4c

if cd $clone_dir; then
	git pull
else
	git clone https://github.com/p4lang/p4c.git --recursive $clone_dir
	cd $clone_dir
fi

#Building P4C repo
if cd build; then
	echo "build dir already exist"
else
	mkdir build
	cd build
fi

cmake ..
make -j12

#Getting P4 Programs
echo "************************\
Finding the Driver Repo*********************"
cd $SDE
for i in */
do
        echo "Checking if $i is Driver Repo"
        cd $i
        if git config --get remote.origin.url | grep -q "https://github.com/intel-innersource/networking.ethernet.acceleration.vswitch.p4-sde.p4-driver.git"; then
                echo "$i is the Driver Repo"
                Repo=$i
                break
        fi
        cd $SDE
done

if [ -z "$Repo" ]; then
        echo "Error: NO Driver Repo Found"
	exit 1
fi

p4c=$SDE/p4c/build/p4c-dpdk
sample_app=$SDE_INSTALL/sample_app
rm -rf $sample_app
mkdir $sample_app

PYTHON_VER=`python --version 2>&1 | awk {'print $2'} | awk -F"." {'print $1"."$2'}`

#Generating artifacts
cd $SDE/$i/NAT_SDE_examples/DPDK
for i in *.p4
do
	p4name=${i%.p4}
	mkdir $sample_app/$p4name
	mkdir $sample_app/$p4name/pipe
	echo "**************************************\
Building $i program*********************************"
	$p4c -I p4include -I p4include/dpdk --p4v=16 \
		--p4runtime-files $sample_app/$p4name/p4info.txt \
		-o $sample_app/$p4name/pipe/$p4name.spec \
		--arch psa \
		--bf-rt-schema $sample_app/$p4name/bf-rt.json \
		--context $sample_app/$p4name/pipe/context.json \
		$i 2>&1

	P4_NAME="--p4-name $p4name"
	BF_RT_DIR="--bf-rt-dir $sample_app/$p4name/bf-rt.json"
	CONTEXT_DIR="--context-dir $sample_app/$p4name/pipe/context.json"
	SPEC_DIR="--spec-dir $sample_app/$p4name/pipe/$p4name.spec"

	echo "Stored bf-rt.json   --> $BF_RT_DIR"
	echo "Stored p4info.txt   --> $sample_app/$p4name/p4info.txt"
	echo "Stored context.json --> $CONTEXT_DIR"
	echo "stored spec file    --> $SPEC_DIR"
	echo "**************************************\
Generating conf file for $i program******************"
	python $SDE_INSTALL/lib/python$PYTHON_VER/site-packages/p4testutils/gen_conf.py $P4_NAME $BF_RT_DIR $CONTEXT_DIR $SPEC_DIR
done | tee -a $LOG_FILE
