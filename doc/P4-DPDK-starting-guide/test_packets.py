#!/usr/bin/env python3
from scapy.all import *
import time

def send_receive_test_packets():
    # Create a test packet for exact match table
    exact_pkt = Ether(
        dst="00:11:22:33:44:55",  # Matches exact table entry
        src="AA:BB:CC:DD:EE:FF"   # Matches exact table entry
    ) / IP(
        src="192.168.2.1",        # Doesn't match ternary table
        dst="172.16.0.1",         # Doesn't match ternary table
        proto=6                   # TCP - matches exact table entry
    ) / TCP()

    # Create a test packet for ternary match table
    ternary_pkt = Ether(
        dst="11:11:11:11:11:11",  # Doesn't match exact table
        src="22:22:22:22:22:22"   # Doesn't match exact table
    ) / IP(
        src="192.168.1.10",       # Matches ternary table entry (192.168.1.0/24)
        dst="10.1.2.3",           # Matches ternary table entry (10.0.0.0/8)
        proto=17                  # UDP - matches ternary table entry
    ) / UDP()

    # Create a test packet that doesn't match any table
    nomatch_pkt = Ether(
        dst="33:33:33:33:33:33",
        src="44:44:44:44:44:44"
    ) / IP(
        src="172.16.1.1",
        dst="172.16.2.1",
        proto=1                   # ICMP
    ) / ICMP()

    # Send and sniff on interface veth0
    print("Sending exact match test packet...")
    sendp(exact_pkt, iface="veth0", verbose=0)
    time.sleep(1)
    
    print("Sending ternary match test packet...")
    sendp(ternary_pkt, iface="veth0", verbose=0)
    time.sleep(1)
    
    print("Sending non-matching test packet...")
    sendp(nomatch_pkt, iface="veth0", verbose=0)
    time.sleep(1)
    
    # Capture packets on veth1
    print("Capturing packets on veth1...")
    pkts = sniff(iface="veth1", count=3, timeout=5)
    
    # Analyze the received packets
    for i, pkt in enumerate(pkts):
        print(f"\nReceived packet {i+1}:")
        if i == 0:
            # This should be the exact match packet with modified source MAC
            print(f"Source MAC: {pkt[Ether].src}")
            print(f"Destination MAC: {pkt[Ether].dst}")
            if pkt[Ether].src == "11:22:33:44:55:66":
                print("SUCCESS: Source MAC was modified by exact match table!")
            else:
                print(f"FAILURE: Source MAC was not modified as expected. Got {pkt[Ether].src}, expected 11:22:33:44:55:66")
        
        elif i == 1:
            # This should be the ternary match packet with modified destination MAC
            print(f"Source MAC: {pkt[Ether].src}")
            print(f"Destination MAC: {pkt[Ether].dst}")
            if pkt[Ether].dst == "66:55:44:33:22:11":
                print("SUCCESS: Destination MAC was modified by ternary match table!")
            else:
                print(f"FAILURE: Destination MAC was not modified as expected. Got {pkt[Ether].dst}, expected 66:55:44:33:22:11")
        
        elif i == 2:
            # This should be the non-matching packet with unchanged MACs
            print(f"Source MAC: {pkt[Ether].src}")
            print(f"Destination MAC: {pkt[Ether].dst}")
            if pkt[Ether].src == "44:44:44:44:44:44" and pkt[Ether].dst == "33:33:33:33:33:33":
                print("SUCCESS: Non-matching packet passed through unchanged!")
            else:
                print(f"FAILURE: Non-matching packet was modified unexpectedly.")
                print(f"Got src={pkt[Ether].src}, dst={pkt[Ether].dst}")
                print(f"Expected src=44:44:44:44:44:44, dst=33:33:33:33:33:33")

if __name__ == "__main__":
    # Set up veth interfaces if they don't exist
    try:
        # Check if veth0 exists
        subprocess.check_call(["ip", "link", "show", "veth0"], 
                             stdout=subprocess.DEVNULL, 
                             stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        # Create veth pair
        print("Creating veth interfaces...")
        subprocess.call(["sudo", "ip", "link", "add", "veth0", "type", "veth", "peer", "name", "veth1"])
        subprocess.call(["sudo", "ip", "link", "set", "dev", "veth0", "up"])
        subprocess.call(["sudo", "ip", "link", "set", "dev", "veth1", "up"])
    
    # Run the test
    send_receive_test_packets()