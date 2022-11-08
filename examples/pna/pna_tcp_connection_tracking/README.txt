The P4 Program for DPDK Connection Tracking is based on :

https://github.com/p4lang/pna/blob/main/examples/pna-example-tcp-connection-tracking.p4

It is modified to include an exact match table to forward packets to particular port in the
DPDK Pipeline based on Destination IP address.

Versioning              : commit-id
==================================================================================================================================================
p4sde                   : c9dc2f256e5ab3ae0d49adb1faa815abb046a557
p4c                     : 026de0abf21ad576ab4e290faa94bfda46d51fce
==================================================================================================================================================

Steps :
========

1. Add Rules to Match TCP Flags in WCM Table and take corresponding actions :

tdi.pna_tcp_connection_tracking.pipe.MainControlImpl.set_ct_options.add_with_tcp_syn_packet(flags=0x02, flags_mask=0x02, MATCH_PRIORITY=53)

tdi.pna_tcp_connection_tracking.pipe.MainControlImpl.set_ct_options.add_with_tcp_fin_or_rst_packet(flags=0x01, flags_mask=0x01, MATCH_PRIORITY=54)

tdi.pna_tcp_connection_tracking.pipe.MainControlImpl.set_ct_options.add_with_tcp_fin_or_rst_packet(flags=0x04, flags_mask=0x04, MATCH_PRIORITY=55)

2. Add Rules to Forward Packets to the Appropriate Ports based on destination IP :

   In this example, we consider TCP Connection between Client (10.10.10.10) and Server (20.20.20.10)

from netaddr import IPAddress
tdi.pna_tcp_connection_tracking.pipe.MainControlImpl.ip_fwd_table.add_with_send(srcAddr=IPAddress('10.10.10.10'), dstAddr=IPAddress('20.20.20.10'), port=2)
tdi.pna_tcp_connection_tracking.pipe.MainControlImpl.ip_fwd_table.add_with_send(dstAddr=IPAddress('10.10.10.10'), srcAddr=IPAddress('20.20.20.10'), port=0)

3. Establish TCP Connection Using Scapy or Iperf :

 Three Way Handshake :

sendp(Ether(dst="00:00:00:aa:aa:aa", src="00:1b:00:09:c5:80")/IP(src="10.10.10.10",dst="20.20.20.10")/TCP(sport=20001,dport=10000,flags="S",seq=0)/Raw(load="0"*50),iface='TAP0')

sendp(Ether(dst="00:1b:00:09:c5:80", src="00:00:00:aa:aa:aa")/IP(src="20.20.20.10",dst="10.10.10.10")/TCP(sport=10000,dport=20001,flags="SA",seq=0, ack=1)/Raw(load="0"*50),iface='TAP2')

sendp(Ether(dst="00:00:00:aa:aa:aa", src="00:1b:00:09:c5:80")/IP(src="10.10.10.10",dst="20.20.20.10")/TCP(sport=20001,dport=10000,flags="A",seq=1,ack=1)/Raw(load="0"*50),iface='TAP0')

 Data Transfer :

sendp(Ether(dst="00:1b:00:09:c5:80", src="00:00:00:aa:aa:aa")/IP(src="20.20.20.10",dst="10.10.10.10")/TCP(sport=10000,dport=20001,flags="",seq=1, ack=1)/Raw(RandString(size=22)),iface='TAP2')

sendp(Ether(dst="00:00:00:aa:aa:aa", src="00:1b:00:09:c5:80")/IP(src="10.10.10.10",dst="20.20.20.10")/TCP(sport=20001,dport=10000,flags="A",seq=1,ack=23)/Raw(load="0"*50),iface='TAP0')

 Connection Termination :

sendp(Ether(dst="00:00:00:aa:aa:aa", src="00:1b:00:09:c5:80")/IP(src="10.10.10.10",dst="20.20.20.10")/TCP(sport=20001,dport=10000,flags="F",seq=1, ack=23)/Raw(load="0"*50),iface='TAP0')

sendp(Ether(dst="00:1b:00:09:c5:80", src="00:00:00:aa:aa:aa")/IP(src="20.20.20.10",dst="10.10.10.10")/TCP(sport=10000,dport=20001,flags="FA",seq=23, ack=1)/Raw(load="0"*50),iface='TAP2')

sendp(Ether(dst="00:00:00:aa:aa:aa", src="00:1b:00:09:c5:80")/IP(src="10.10.10.10",dst="20.20.20.10")/TCP(sport=20001,dport=10000,flags="A",seq=1,ack=24)/Raw(load="0"*50),iface='TAP0')

4. Look for debugs on bf_switchd console for Auto-learning of Flows in Add-on-miss table :

 Table Miss :
 ================

 ------------------ Learner Debug Stats------------------------------
Received input key is:
  Dump data at [0x56148c748cfc], len=20
00000000: 06 01 0A 0A 0A 01 64 0A 0A 21 4E 10 27 00 00 00 | ......d..!N.'...
00000010: 00 00 01 00                                     | ....
--------------------------------------------------------------------
Searching following key in the bucket.
  Dump data at [0x56148c748cfc], len=20
00000000: 06 01 0A 0A 0A 01 64 0A 0A 21 4E 10 27 00 00 00 | ......d..!N.'...
00000010: 00 00 01 00                                     | ....
Key was not Found!
Action status : miss (Default Action)
Action to be taken is : ct_tcp_table_miss
--------------------------------------------------------------------
Adding the missed key to the bucket.
Key added to the bucket.
--------------------------------------------------------------------

 Table Hit :
 ================

------------------ Learner Debug Stats------------------------------
Received input key is:
  Dump data at [0x56148c748dec], len=20
00000000: 06 01 0A 0A 0A 01 64 0A 0A 21 4E 10 27 00 00 00 | ......d..!N.'...
00000010: 00 00 01 00                                     | ....
--------------------------------------------------------------------
Searching following key in the bucket.
  Dump data at [0x56148c748dec], len=20
00000000: 06 01 0A 0A 0A 01 64 0A 0A 21 4E 10 27 00 00 00 | ......d..!N.'...
00000010: 00 00 01 00                                     | ....
Key Found Operation Successful!
The key position in the bucket is: 0
Action status : hit (Learnt Action)
Action to be taken is : ct_tcp_table_hit
--------------------------------------------------------------------

5. Based on the state of the TCP Connection, different timeout value will be assigned to the
   auto-learned flow. After timeout expires, flow will be removed from the table.
