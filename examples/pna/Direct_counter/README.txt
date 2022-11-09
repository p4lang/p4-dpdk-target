#P4C artifacts generation--------------------------------------------------------------------------------------------------------------------------- 
#building P4 compiler
git clone --recursive https://github.com/p4lang/p4c.git
git submodule update --init --recursive
mkdir build
cd build
cmake .. 
make -j4
#artifacts generation
mkdir example
cd example
#copy counter.p4 
#run below command to generate artifacts
<p4 compiler repo>/p4c/build/p4c-dpdk --arch pna counter.p4 -o counter.p4.spec --context context.json --bf-rt-schema tdi.json --p4runtime-files p4info.txt

#P4SDE driver--------------------------------------------------------------------------------------------------------------------------------------
#set env var (console-1)
export SDE=<sde_repo path>
export SDE_INSTALL=$SDE/install
export LD_LIBRARY_PATH=$SDE_INSTALL/lib:$SDE_INSTALL/lib64 
export PYTHONPATH=$SDE_INSTALL/lib/python3.10/:$SDE_INSTALL/lib/python3.10/lib-dynload:$SDE_INSTALL/lib/python3.10/site-packages
export PYTHONHOME=$SDE_INSTALL/lib/python3.10

#copy the counters example dir in $SDE_INSTALL/share

#run bf_switchd application
cd $SDE_INSTALL/bin
./bf_switchd --install-dir $SDE_INSTALL --conf-file $SDE_INSTALL/share/counter/counter.conf --init-mode=cold --status-port 7777 --skip-hld mkt

#configure rules.
tdi_python
tdi.enable
tdi.counter.pipe.MainControlImpl.ipv4_host.add_with_send(dstAddr="10.10.20.100", port=3, COUNTER_SPEC_PKTS=0)

#rule 1 will get counter id 1

#commands on new console (console-2)
make tap interfaces up
tcpdump -i TAP3 -XXx

#commands on scapy console (console-3)
scapy
#send the traffic from TAP0 to TAP3 interface to match each rule to see the corresponding counter increment.
sendp(Ether(dst="aa:bb:cc:dd:00:00", src="9e:ba:ce:98:d9:d3")/IP(src="192.168.1.10", dst="10.10.20.100")/TCP(sport=7777, dport=8888)/Raw(load="1"*50), iface='TAP0')

#To check the counter stats go to console-1
get(dstAddr="10.10.20.100")

#Expected result
tdi.counter.pipe.MainControlImpl.ipv4_host> get(dstAddr="10.10.20.100")
[Thread 0x7fffdd27d640 (LWP 2293988) exited]
Entry 0:
Entry key:
    hdr.ipv4.dstAddr               : 0x0A0A1464
Entry data (action : MainControlImpl.send):
    port                           : 0x00000003
    $COUNTER_SPEC_PKTS             : 1

Out[8]: Entry for pipe.MainControlImpl.ipv4_host table.

