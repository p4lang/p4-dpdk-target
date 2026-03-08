#!/usr/bin/env bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
NC='\033[0m' # No Color

print_step() {
    echo -e "\n${BOLD}${BLUE}==>${NC} ${BOLD}$1${NC}"
}

echo -e "${BOLD}${BLUE}=== P4 DPDK Target Installation Script ===${NC}"

check_and_install() {
    local cmd=$1
    local pkgs=$2

    # Check if command exists (for executables)
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo -e "${YELLOW}$cmd not found.${NC}"
        sudo apt install -y $pkgs
    fi
 }

check_and_install_pkgs() {
     local pkgs=$1
    if dpkg-query -l "$pkgs" >/dev/null 2>&1; then
        echo -e "${GREEN}$pkgs is already installed.${NC}"
    else
        echo -e "${YELLOW}$pkgs not found. Installing...${NC}"
        sudo apt install -y "$pkgs"
    fi
}

install_deps_ubuntu_2404() {
    print_step "Checking Ubuntu version..."
    if [ -f /etc/os-release ]; then
        source /etc/os-release
        if [[ "$VERSION_ID" == "24.04" ]]; then
            IS_24_04="true"
        else
            IS_24_04="false"
        fi
    fi
}


# ------------------------------------------------------------
# 1. Base directories
# ------------------------------------------------------------
export SDE=$(pwd)/sde
export SDE_INSTALL=$SDE/install

mkdir -p "$SDE"
mkdir -p "$SDE_INSTALL"

# ------------------------------------------------------------
# 0. Check Dependencies (install if missing)
# ------------------------------------------------------------
print_step "0/5 Checking dependencies..."

sudo apt-get update -y && sudo apt-get upgrade -y

# Install Wireshark and tshark on Ubuntu system without having to
# answer _any_ questions interactively, except perhaps providing your
# password when prompted by 'sudo'.
# https://askubuntu.com/questions/1275842/install-wireshark-without-confirm
echo "wireshark-common wireshark-common/install-setuid boolean true" | sudo debconf-set-selections
sudo DEBIAN_FRONTEND=noninteractive apt-get -y install wireshark tshark

install_deps_ubuntu_2404

if [[ "$IS_24_04" == "true" ]]; then
    sudo apt install -y libedit-dev pkg-config
fi

check_and_install "pip3" "python3-pip"
check_and_install "autoreconf" "autoconf automake libtool pkg-config autoconf-archive automake"
check_and_install "cmake" "cmake" 
check_and_install "doxygen" "doxygen" 
check_and_install "dpdk-testpmd" "dpdk dpdk-dev libdpdk-dev"
check_and_install_pkgs "libcjson-dev"
check_and_install_pkgs "libedit-dev"
check_and_install_pkgs "libffi-dev"

# ------------------------------------------------------------
# 1. Cloning p4-dpdk-target
# ------------------------------------------------------------
print_step "1/5 Cloning p4-dpdk-target..."
cd $SDE
    git clone https://github.com/p4lang/p4-dpdk-target.git

# ------------------------------------------------------------
# 2. System dependencies
# ------------------------------------------------------------
print_step "2/5 Installing system dependencies..."

sudo apt-get install -y python3-venv python3-full
python3 -m venv "$SDE/.venv"
source "$SDE/.venv/bin/activate"

cd "$SDE/p4-dpdk-target/tools/setup"
source ./p4sde_env_setup.sh "$SDE"
if [[ "$IS_24_04" == "true" ]]; then
    pip3 install setuptools
fi
pip3 install distro
python3 install_dep.py

# ------------------------------------------------------------
# 3. Build p4-dpdk-target
# ------------------------------------------------------------
print_step "3/5 Building p4-dpdk-target..."

cd $SDE/p4-dpdk-target
git submodule update --init --recursive --force
./autogen.sh
read -p "Build for TDI only? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    ./configure --prefix=$SDE_INSTALL --with-generic-flags=yes #For TDI build
else
    ./configure --prefix=$SDE_INSTALL #For both bfrt and TDI build
fi
make -j
make install

# ------------------------------------------------------------
# 4. Hugepages (required for DPDK)
# ------------------------------------------------------------
print_step "4/5 Configuring hugepages..."
read -p "Configure hugepages? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo sysctl -w vm.nr_hugepages=1024 || true
    sudo mkdir -p /mnt/huge || true
    mount | grep hugetlbfs || sudo mount -t hugetlbfs nodev /mnt/huge || true
else
    echo "Skipping hugepages configuration."
fi

# ------------------------------------------------------------
# 5. Hugepages permissions
# ------------------------------------------------------------
print_step "5/5 Configuring hugepages permissions..."
read -p "Configure hugepages permissions? [y/N] " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo chown $(id -u):$(id -g) /dev/hugepages
    sudo chmod 700 /dev/hugepages
else
    echo "Skipping hugepages permissions configuration."
fi

echo -e "${BOLD}${GREEN}DONE ✔${NC}"
