#!/usr/bin/env python3

import time
import unittest

from scapy.all import IP, UDP, AsyncSniffer, Ether, sendp


class ScapyTestCase(unittest.TestCase):
    WAIT_TIME = 2

    # TODO: Make the following test pass.
    # def test_one_packet_received_at_expected_port(self):
    #     sniffer = AsyncSniffer(iface="TAP1", count=8)
    #     sniffer.start()

    #     pkt = Ether() / IP() / UDP() / "Hello, World!"
    #     sendp(pkt, iface="TAP0")
    #     pkt_sent = pkt

    #     time.sleep(self.WAIT_TIME)
    #     pkts_recvd = sniffer.stop()

    #     self.assertEqual(len(pkts_recvd), 1)
    #     pkt_recvd = pkts_recvd[0]
    #     self.assertEqual(pkt_recvd, pkt_sent)

    def test_no_packet_received_at_other_ports(self):
        sniffer = AsyncSniffer(iface=["TAP2", "TAP3"], count=8)
        sniffer.start()

        pkt = Ether() / IP() / UDP() / "Hello, World!"
        sendp(pkt, iface="TAP0")

        time.sleep(self.WAIT_TIME)
        pkts_recvd = sniffer.stop()

        self.assertEqual(len(pkts_recvd), 0)


if __name__ == "__main__":
    unittest.main()
