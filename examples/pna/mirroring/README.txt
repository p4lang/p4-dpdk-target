#set env var 
export SDE=<sde repo path>
export SDE_INSTALL=$SDE/install
export LD_LIBRARY_PATH=$SDE_INSTALL/lib:$SDE_INSTALL/lib64
export LD_LIBRARY_PATH=$SDE_INSTALL/lib/:$SDE_INSTALL/lib/x86_64-linux-gnu/:$SDE_INSTALL/lib64
export PKG_CONFIG_PATH=$SDE_INSTALL/lib/x86_64-linux-gnu/pkgconfig
export PYTHONPATH=$SDE_INSTALL/lib/python3.8/:$SDE_INSTALL/lib/python3.8/lib-dynload:$SDE_INSTALL/lib/python3.8/site-packages/
export PYTHONHOME=$SDE_INSTALL/lib/python3.8

#configure huge pages
echo 16 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

#copy the mirror_valuelookup table example dir in $SDE_INSTALL/share

#run bf_switchd application
cd $SDE_INSTALL/bin
./bf_switchd --install-dir $SDE_INSTALL --conf-file $SDE_INSTALL/share/mirror_valuelookup/mirror_valuelookup.conf --init-mode=cold --status-port 7777 --skip-hld mkt 
tdi_python
tdi.enable()

#In another terminal bring up the configured TAP interfaces.
sudo ip link set TAP0 up
sudo ip link set TAP1 up
sudo ip link set TAP2 up
sudo ip link set TAP3 up

#Configure rules in bf_switchd bf_shell cli
tdi.mirror_valuelookup.pipe.my_control.mir_prof.add(id=1,store_port=2,trunc_size=256) # rule for mirror session
tdi.mirror_valuelookup.pipe.my_control.fwd.add_with_mirror_and_send(dstAddr=0x00050004a731, srcAddr=0x00050004a732, port=1, mirror_session_id=1) # rule for l2_fwd

#start scapy and send traffic
sendp(Ether(dst="00:05:00:04:a7:31", src="00:05:00:04:a7:32")/Raw(load="0"*500), iface='TAP0')

#start traffic capture on port1 and port2 to see the original pkt with 500bytes payload and mirrored pkt 256bytes payload.
tcpdump -XXi <iface>

#LIMITATION
Currently mirror_packet p4 construct does not take the mirror session id param as a variable, but it takes a hard coded value, due to this the spec file always contains the hard coded mirror session param, we need to change that in action mirror_and_send (mov m.mirrorSession t.mirror_session_id).
