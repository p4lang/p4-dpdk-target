# Adding Table Entries to P4 DPDK Using Native Control Plane API

This guide demonstrates how to add table entries to P4 DPDK using its native control plane API. We'll implement a simple P4 program with two tables, create a control plane program to add entries to these tables, and test the setup with packet traffic.

## Table of Contents
- [P4 Program](#p4-program)
- [Control Plane Program](#control-plane-program)
- [Setup and Compilation](#setup-and-compilation)
- [Testing with Scapy](#testing-with-scapy)
- [Troubleshooting](#troubleshooting)

## P4 Program

First, let's create a simple P4 program (`mac_modifier.p4`) with two tables:
1. An exact match table that modifies the source MAC address
2. A ternary match table that modifies the destination MAC address

Use the mac_modifier.p4 given in the P4-DPDK-starting-guide directory

This P4 program defines:
- An Ethernet and IPv4 header structure
- An exact match table with three keys: destination MAC, source MAC, and IP protocol
- A ternary match table with three keys: source IP, destination IP, and IP protocol
- Actions to modify source and destination MAC addresses
- Default actions set to NoAction (packets not matching any entry proceed unchanged)

## Control Plane Program

Now, let's create a control plane program (`table_manager.c`) to add entries to our tables using the P4 Runtime API:

Use the control plane program table_manager.c given in the same directory 


This control plane program:
1. Initializes the DPDK environment
2. Creates a P4 Runtime context
3. Adds an entry to the exact match table:
   - Matches packets with destination MAC 00:11:22:33:44:55, source MAC AA:BB:CC:DD:EE:FF, and TCP protocol (6)
   - Sets the source MAC to 11:22:33:44:55:66
4. Adds an entry to the ternary match table:
   - Matches packets with source IP in the 192.168.1.0/24 subnet, destination IP in the 10.0.0.0/8 subnet, and UDP protocol (17)
   - Sets the destination MAC to 66:55:44:33:22:11
5. Reads and prints all table entries
6. Cleans up resources

## Setup and Compilation

Let's create a Makefile to build our P4 program and control plane program:

```makefile
# Makefile for P4 DPDK MAC Modifier example

# Define variables
P4C = p4c-dpdk
P4_SRC = mac_modifier.p4
P4_JSON = mac_modifier.json
P4_CLI = mac_modifier_cli.txt
P4_DPDK_TARGET = mac_modifier.spec

CC = gcc
CFLAGS = -Wall -Werror
LDFLAGS = -lrte_eal -lrte_mempool -lrte_mbuf -lrte_ethdev -lp4rt
CONTROL_SRC = table_manager.c
CONTROL_BIN = table_manager

# Default target
all: p4_compile control_compile

# Compile P4 program
p4_compile:
	$(P4C) --arch psa --target dpdk -o $(P4_DPDK_TARGET) $(P4_SRC)

# Compile control plane program
control_compile:
	$(CC) $(CFLAGS) -o $(CONTROL_BIN) $(CONTROL_SRC) $(LDFLAGS)

# Clean generated files
clean:
	rm -f $(P4_JSON) $(P4_DPDK_TARGET) $(CONTROL_BIN)

.PHONY: all p4_compile control_compile clean
```

### Setup Instructions

1. **Install Prerequisites**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake python3-pip
   pip3 install scapy
   ```

2. **Install DPDK**:
   ```bash
   git clone https://github.com/DPDK/dpdk.git
   cd dpdk
   git checkout v21.11
   meson build
   cd build
   ninja
   sudo ninja install
   sudo ldconfig
   ```

3. **Install P4C (P4 Compiler)**:
   ```bash
   git clone https://github.com/p4lang/p4c.git
   cd p4c
   git submodule update --init --recursive
   mkdir build
   cd build
   cmake ..
   make -j4
   sudo make install
   ```

4. **Set Up P4 DPDK Runtime**:
   ```bash
   git clone https://github.com/p4lang/p4-dpdk-target.git
   cd p4-dpdk-target
   mkdir build
   cd build
   cmake ..
   make -j4
   sudo make install
   ```

5. **Create Project Directory**:
   ```bash
   mkdir -p ~/p4_dpdk_example
   cd ~/p4_dpdk_example
   ```

6. **Create the P4 and Control Plane Files**:
   - Save the P4 program as `mac_modifier.p4`
   - Save the control plane program as `table_manager.c`
   - Save the Makefile as `Makefile`

7. **Compile the Programs**:
   ```bash
   make
   ```

### Configure DPDK

1. **Set Up Hugepages**:
   ```bash
   sudo mkdir -p /mnt/huge
   sudo mount -t hugetlbfs nodev /mnt/huge
   echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
   ```

2. **Bind Network Interfaces to DPDK**:
   ```bash
   # Find available network interfaces
   sudo dpdk-devbind.py --status

   # Bind interface to DPDK (replace X with your interface number)
   sudo dpdk-devbind.py --bind=vfio-pci 0000:XX:XX.X
   ```

## Running the Example

1. **Start the P4 DPDK Switch**:
   ```bash
   sudo dpdk-p4-switch --log-level=debug --no-pci \
     --vdev="net_pcap0,iface=veth0" \
     --vdev="net_pcap1,iface=veth1" \
     -p mac_modifier.spec
   ```

2. **Run the Control Plane Program**:
   ```bash
   sudo ./table_manager
   ```

3. **Verify Table Entries**:
   - The control plane program should print the added table entries.

## Testing with Scapy

Create a Python script (`test_packets.py`) to send and receive test packets:

```python
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
```

Make the script executable:
```bash
chmod +x test_packets.py
```

### Running the Test

1. **Create Virtual Interfaces** (if not already done in the script):
   ```bash
   sudo ip link add veth0 type veth peer name veth1
   sudo ip link set dev veth0 up
   sudo ip link set dev veth1 up
   ```

2. **Run the Test Script**:
   ```bash
   sudo ./test_packets.py
   ```

3. **Verify the Results**:
   - The first packet should have its source MAC modified to 11:22:33:44:55:66
   - The second packet should have its destination MAC modified to 66:55:44:33:22:11
   - The third packet should pass through unchanged

## Troubleshooting

If you encounter issues, check the following:

1. **P4 Compilation Errors**:
   - Verify P4 syntax
   - Ensure header and field definitions match usage in tables

2. **Control Plane Connection Issues**:
   - Check if the P4 DPDK switch is running
   - Verify P4 Runtime API paths and function calls

3. **Packet Processing Issues**:
   - Confirm network interfaces are correctly bound to DPDK
   - Check table entry formats (especially for ternary entries)
   - Enable debug logging with `--log-level=debug`

4. **Common Issues and Solutions**:

   - **Issue**: Table entries not applying
     **Solution**: Verify field names match exactly with the P4 program

   - **Issue**: P4 DPDK switch crashes
     **Solution**: Check memory allocation and hugepages setup

   - **Issue**: Packets not being received
     **Solution**: Check interface configuration and ensure packets are being sent on the correct interface

   - **Issue**: Control plane connection errors
     **Solution**: Make sure the P4 DPDK service is running and listening on the expected socket

   - **Issue**: Ternary matches not working
     **Solution**: Double-check mask values and ensure priorities are set
     **Solution**: Check interface configuration and ensure packets are being sent on the correct interface

   - **Issue**: Control plane connection errors
     **Solution**: Make sure the P4 DPDK service is running and listening on the expected socket

   - **Issue**: Ternary matches not working
     **Solution**: Double-check mask values and ensure priorities are set correctly

5. **Debugging Commands**:
   ```bash
   # Check P4 DPDK process status
   ps aux | grep dpdk-p4-switch
   
   # View P4 DPDK logs
   sudo tail -f /var/log/dpdk-p4-switch.log
   
   # Check DPDK interface bindings
   sudo dpdk-devbind.py --status
   
   # Test raw packet sending
   sudo tcpreplay -i veth0 test_packet.pcap
   ```

## Complete Example Walkthrough

Let's walk through the whole process step by step:

### 1. Setup Environment

```bash
# Create project directory
mkdir -p ~/p4_dpdk_example
cd ~/p4_dpdk_example

# Create P4 program, control plane program, Makefile, and test script
# (copy contents from earlier sections)

# Set up hugepages
sudo mkdir -p /mnt/huge
sudo mount -t hugetlbfs nodev /mnt/huge
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

# Create virtual interfaces
sudo ip link add veth0 type veth peer name veth1
sudo ip link set dev veth0 up
sudo ip link set dev veth1 up
```

### 2. Compile Programs

```bash
# Compile P4 program and control plane
make

# Confirm files exist
ls -la mac_modifier.spec table_manager
```

### 3. Run P4 DPDK Switch

```bash
# Start P4 DPDK switch in a terminal
sudo dpdk-p4-switch --log-level=debug --no-pci \
  --vdev="net_pcap0,iface=veth0" \
  --vdev="net_pcap1,iface=veth1" \
  -p mac_modifier.spec
```

### 4. Add Table Entries

```bash
# In another terminal, run the control plane program
sudo ./table_manager

# Expected output should show successful table entry additions
# and list the entries that were added to each table
```

### 5. Test with Packet Traffic

```bash
# Run the test script
sudo ./test_packets.py

# Expected output:
# - SUCCESS: Source MAC was modified by exact match table!
# - SUCCESS: Destination MAC was modified by ternary match table!
# - SUCCESS: Non-matching packet passed through unchanged!
```

## Making Changes and Experimenting

Now that you have a working example, you can experiment with:

1. **Modifying Table Keys**:
   - Add or remove match fields
   - Change match types (exact, ternary, LPM)

2. **Extending Actions**:
   - Modify more packet fields
   - Add conditional actions

3. **Adding More Tables**:
   - Create tables for different header fields
   - Implement table chaining logic

4. **Performance Testing**:
   - Benchmark with different packet sizes
   - Test with high packet rates

5. **Error Handling**:
   - Add error handling in the control plane
   - Test boundary conditions

When making changes to the P4 program, remember to recompile and restart the P4 DPDK switch. For control plane changes, just recompile the control plane program.

## Conclusion

This guide has demonstrated how to:
1. Create a P4 program with exact and ternary match tables
2. Implement a control plane program to add table entries
3. Set up a P4 DPDK environment
4. Test the implementation with real packet traffic

By following these steps, you've created a functioning P4 DPDK data plane with a native control plane. This serves as a foundation for more complex networking applications using P4 DPDK.
