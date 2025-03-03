# Adding Table Entries to P4 DPDK Using Native Control Plane API

This guide demonstrates how to add table entries to P4 DPDK using its native control plane API. We'll implement a simple P4 program with two tables, create a control plane program to add entries to these tables, and test the setup with packet traffic.

## Table of Contents
- [P4 Program](#p4-program)
- [Control Plane Program](#control-plane-program)
- [Setup and Compilation](#setup-and-compilation)
- [Installing P4RT (P4 Runtime)](#installing-p4rt-p4-runtime)
- [Configure DPDK](#configure-dpdk)
- [Running the Example](#running-the-example)
- [Testing with Scapy](#testing-with-scapy)
- [Troubleshooting](#troubleshooting)
- [Complete Example Walkthrough](#complete-example-walkthrough)

## P4 Program

First, let's create a simple P4 program (`mac_modifier.p4`) with two tables:
1. An exact match table that modifies the source MAC address
2. A ternary match table that modifies the destination MAC address

Use the mac_modifier.p4 given in the examples/mac_modifier directory.

This P4 program defines:
- An Ethernet and IPv4 header structure
- An exact match table with three keys: destination MAC, source MAC, and IP protocol
- A ternary match table with three keys: source IP, destination IP, and IP protocol
- Actions to modify source and destination MAC addresses
- Default actions set to NoAction (packets not matching any entry proceed unchanged)

## Control Plane Program

Now, let's create a control plane program (`table_manager.c`) to add entries to our tables using the P4 Runtime API:

Use the control plane program given in the e2e-test/p4rt directory.

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
Use the makefile provided in the e2e-test/p4rt directory 


### Setup Instructions

1. **Install Prerequisites**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y build-essential cmake python3-pip pkg-config \
       libpcap-dev libnl-3-dev libnl-genl-3-dev libnl-route-3-dev \
       libboost-dev libboost-program-options-dev libboost-system-dev \
       libboost-filesystem-dev libboost-thread-dev libboost-test-dev \
       libjudy-dev libgmp-dev libreadline-dev
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

## Installing P4RT (P4 Runtime)

P4RT is the control plane API that allows you to manage P4 tables programmatically. Follow these steps to install it:

### 1. Install Protocol Buffers

P4RT requires Protocol Buffers (protobuf) version 3.6.1 or later:

```bash
cd ~/
git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf
git checkout v3.18.0
./autogen.sh
./configure
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 2. Install gRPC

P4RT uses gRPC for communication:

```bash
cd ~/
git clone https://github.com/grpc/grpc.git
cd grpc
git checkout v1.42.0
git submodule update --init --recursive
mkdir -p cmake/build
cd cmake/build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DgRPC_PROTOBUF_PROVIDER=package \
      -DgRPC_ZLIB_PROVIDER=package \
      -DgRPC_CARES_PROVIDER=package \
      -DgRPC_SSL_PROVIDER=package \
      ../..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 3. Install P4Runtime

Clone and install the P4Runtime library:

```bash
cd ~/
git clone https://github.com/p4lang/p4runtime.git
cd p4runtime
git checkout v1.3.0
mkdir -p build
cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 4. Install PI (P4 Interface)

P4 Interface is needed for P4Runtime implementation:

```bash
cd ~/
git clone https://github.com/p4lang/PI.git
cd PI
git checkout v0.9.0
git submodule update --init --recursive
./autogen.sh
./configure --with-proto --with-p4runtime --without-bmv2 --without-cli
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 5. Install the P4RT DPDK library

Now install the specific P4RT DPDK library:

```bash
cd ~/
git clone https://github.com/p4lang/p4rt-dpdk.git
cd p4rt-dpdk
mkdir -p build
cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

### 6. Verify P4RT Installation

Verify that the P4RT library is correctly installed:

```bash
pkg-config --modversion libp4rt
```

This command should return the version of the installed P4RT library. If it fails, ensure that the PKG_CONFIG_PATH environment variable includes the path to the P4RT .pc file:

```bash
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
```

### 7. Create Project Directory

```bash
mkdir -p ~/p4_dpdk_example
cd ~/p4_dpdk_example
```

### 8. Create the P4 and Control Plane Files

- Save the P4 program as `mac_modifier.p4`
- Save the control plane program as `table_manager.c`
- Save the Makefile as `Makefile`

### 9. Compile the Programs

```bash
make
```

## Configure DPDK

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

Create a Python script (`test_packets.py`) to send and receive test packets. Use test_packets.py provided in the e2e-test/p4rt directory.

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

4. **P4RT Installation Issues**:
   - Missing Libraries: If you encounter errors about missing libraries during compilation, use `ldd` to check which libraries are missing:
     ```bash
     ldd ./table_manager
     ```
   - Library Path: If the linker cannot find libraries at runtime, update the LD_LIBRARY_PATH:
     ```bash
     export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
     ```
   - Include Paths: If you see header file not found errors, you may need to add additional include paths in your Makefile:
     ```makefile
     CFLAGS += -I/usr/local/include/p4rt
     ```

5. **Common Issues and Solutions**:

   - **Issue**: Table entries not applying
     **Solution**: Verify field names match exactly with the P4 program

   - **Issue**: P4 DPDK switch crashes
     **Solution**: Check memory allocation and hugepages setup

   - **Issue**: Packets not being received
     **Solution**: Check interface configuration and ensure packets are being sent on the correct interface

   - **Issue**: Control plane connection errors
     **Solution**: Make sure the P4 DPDK service is running and listening on the expected socket

   - **Issue**: Ternary matches not working
     **Solution**: Double-check mask values and ensure priorities are set correctly

6. **Debugging Commands**:
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
# Install all prerequisites
sudo apt-get update
sudo apt-get install -y build-essential cmake python3-pip pkg-config \
    libpcap-dev libnl-3-dev libnl-genl-3-dev libnl-route-3-dev \
    libboost-dev libboost-program-options-dev libboost-system-dev \
    libboost-filesystem-dev libboost-thread-dev libboost-test-dev \
    libjudy-dev libgmp-dev libreadline-dev
pip3 install scapy

# Install DPDK, P4C, P4 DPDK Runtime, and P4RT
# (Follow instructions from previous sections)

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
1. Set up the complete environment including P4RT
2. Create a P4 program with exact and ternary match tables
3. Implement a control plane program to add table entries
4. Set up a P4 DPDK environment
5. Test the implementation with real packet traffic

By following these steps, you've created a functioning P4 DPDK data plane with a native control plane using P4RT. This serves as a foundation for more complex networking applications using P4 DPDK.