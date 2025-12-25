#!/usr/bin/env bash
set -e

echo "=== P4 DPDK Target Installation Script ==="

# ------------------------------------------------------------
# 0. Base directories
# ------------------------------------------------------------
export SDE=$HOME/sde
export SDE_INSTALL=$SDE/install

mkdir -p "$SDE"
mkdir -p "$SDE_INSTALL"

# ------------------------------------------------------------
# 1. System dependencies
# ------------------------------------------------------------
echo "[1/9] Installing system dependencies..."

sudo apt update
sudo apt install -y \
  git curl unifdef jq \
  python3 python3-dev python3-venv python3-pip \
  python3-setuptools python3-wheel python3-cffi \
  libconfig-dev libunwind-dev libffi-dev \
  zlib1g-dev libedit-dev libexpat1-dev \
  clang gcc g++ libstdc++6 \
  autoconf automake autotools-dev autoconf-archive libtool \
  cmake ninja-build meson \
  google-perftools connect-proxy tshark \
  dpdk dpdk-dev libdpdk-dev

# ------------------------------------------------------------
# 2. Hugepages (required for DPDK)
# ------------------------------------------------------------
echo "[2/9] Configuring hugepages..."

sudo sysctl -w vm.nr_hugepages=1024 || true
sudo mkdir -p /mnt/huge || true
mount | grep hugetlbfs || sudo mount -t hugetlbfs nodev /mnt/huge || true

# ------------------------------------------------------------
# 3. Python virtual environment for build tools
# ------------------------------------------------------------
echo "[3/9] Creating Python virtual environment..."

cd "$SDE"
python3 -m venv venv
source venv/bin/activate

pip install --upgrade pip
pip install \
  distro \
  thrift \
  protobuf \
  pyelftools \
  scapy \
  six

# ------------------------------------------------------------
# 4. Clone repositories
# ------------------------------------------------------------
echo "[4/9] Cloning repositories..."

cd "$SDE"

git clone --depth=1 https://github.com/p4lang/target-utils utils
git clone --depth=1 https://github.com/p4lang/target-syslibs syslibs
git clone --depth=1 https://github.com/p4lang/p4-dpdk-target p4-dpdk-target

# ------------------------------------------------------------
# 5. Install dependencies via helper script
# ------------------------------------------------------------
echo "[5/9] Running install_dep.py..."

cd "$SDE/p4-dpdk-target/tools/setup"
python3 install_dep.py

# ------------------------------------------------------------
# 6. Build target-utils
# ------------------------------------------------------------
echo "[6/9] Building target-utils..."

cd "$SDE/utils"
mkdir -p build
cd build

cmake \
  -DCMAKE_INSTALL_PREFIX="$SDE_INSTALL" \
  -DCPYTHON=1 \
  -DSTANDALONE=ON \
  ..

make -j$(nproc)
make install

# ------------------------------------------------------------
# 7. Build target-syslibs
# ------------------------------------------------------------
echo "[7/9] Building target-syslibs..."

cd "$SDE/syslibs"
mkdir -p build
cd build

cmake -DCMAKE_INSTALL_PREFIX="$SDE_INSTALL" ..
make -j$(nproc)
make install

# ------------------------------------------------------------
# 8. Build p4-dpdk-target
# ------------------------------------------------------------
echo "[8/9] Building p4-dpdk-target..."

cd "$SDE/p4-dpdk-target"

git submodule update --init --recursive
./autogen.sh
./configure --prefix="$SDE_INSTALL"
make -j$(nproc)
make install

# ------------------------------------------------------------
# 9. Runtime environment setup
# ------------------------------------------------------------
echo "[9/9] Setting runtime environment variables..."

cat <<EOF

============================================================
Installation complete.

Add the following to your ~/.bashrc or source it manually:

export SDE=\$HOME/sde
export SDE_INSTALL=\$SDE/install
export LD_LIBRARY_PATH=\$SDE_INSTALL/lib:\$SDE_INSTALL/lib64:\$SDE_INSTALL/lib/x86_64-linux-gnu
export PATH=\$SDE_INSTALL/bin:\$PATH
export PYTHONPATH=\$SDE_INSTALL/lib/python3.10/site-packages
export PYTHONHOME=\$SDE_INSTALL/lib/python3.10

============================================================

EOF

echo "DONE ✔"
