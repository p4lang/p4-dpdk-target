#! /bin/bash

# Remember the current directory when the script was started:
SCRIPT_START_DIR="${PWD}"

THIS_SCRIPT_FILE_MAYBE_RELATIVE="$0"
THIS_SCRIPT_DIR_MAYBE_RELATIVE="${THIS_SCRIPT_FILE_MAYBE_RELATIVE%/*}"
THIS_SCRIPT_DIR_ABSOLUTE=`readlink -f "${THIS_SCRIPT_DIR_MAYBE_RELATIVE}"`

os_version_warning() {
    1>&2 echo "This software has only been tested on these systems:"
    1>&2 echo "    Fedora 34"
    1>&2 echo "    Ubuntu 22.04"
    1>&2 echo ""
    1>&2 echo "Proceed installing manually at your own risk of"
    1>&2 echo "significant time spent figuring out how to make it all"
    1>&2 echo "work, or consider getting VirtualBox and creating an"
    1>&2 echo "appropriate virtual machine with one of the tested versions."
}

if [ ! -e /etc/os-release ]
then
    1>&2 echo "No file /etc/os-release found.  Aborting because we have"
    1>&2 echo "no way to tell which Linux distribution and version you"
    1>&2 echo "are running."
    exit 1
fi

supported_os=0
source /etc/os-release
if [ "${ID}" = "ubuntu" -a \( "${VERSION_ID}" = "22.04" \) ]
then
    echo "Found distribution '${ID}' version '${VERSION_ID}'.  Continuing with installation."
    supported_os=1
fi
if [ "${ID}" = "fedora" -a \( "${VERSION_ID}" = "34" -o "${VERSION_ID}" = "36" \) ]
then
    echo "Found distribution '${ID}' version '${VERSION_ID}'.  Continuing with installation."
    supported_os=1
fi

if [ $supported_os == 0 ]
then
    os_version_warning
    1>&2 echo ""
    1>&2 echo "Here are the contents of /etc/os-release:"
    cat /etc/os-release
    exit 1
fi

# Minimum required system memory is 4 GBytes, minus a few MBytes
# because from experiments I have run on several different Ubuntu
# Linux VMs, when you configure them with 2 Gbytes of RAM, the first
# line of /proc/meminfo shows a little less than that available, I
# believe because some memory occupied by the kernel is not shown.

min_mem_MBytes=`expr 4 \* \( 1024 - 64 \)`
memtotal_KBytes=`head -n 1 /proc/meminfo | awk '{print $2;}'`
memtotal_MBytes=`expr ${memtotal_KBytes} / 1024`

if [ "${memtotal_MBytes}" -lt "${min_mem_MBytes}" ]
then
    memtotal_comment="too low"
    abort_script=1
else
    memtotal_comment="enough"
fi

echo "Minimum recommended memory to run this script: ${min_mem_MBytes} MBytes"
echo "Memory on this system from /proc/meminfo:      ${memtotal_MBytes} MBytes -> $memtotal_comment"

min_free_disk_MBytes=`expr 6 \* 1024`
free_disk_MBytes=`df --output=avail --block-size=1M . | tail -n 1`

if [ "${free_disk_MBytes}" -lt "${min_free_disk_MBytes}" ]
then
    free_disk_comment="too low"
    abort_script=1
else
    free_disk_comment="enough"
fi

echo "Minimum free disk space to run this script:    ${min_free_disk_MBytes} MBytes"
echo "Free disk space on this system from df output: ${free_disk_MBytes} MBytes -> $free_disk_comment"

if [ "${abort_script}" == 1 ]
then
    echo ""
    echo "Aborting script because system has too little RAM or free disk space"
    exit 1
fi

cd "${THIS_SCRIPT_DIR_ABSOLUTE}"
source p4sde_env_setup.sh $HOME/sde

if [ "${ID}" = "ubuntu" ]
then
    sudo apt update

    # Install Wireshark and tshark on Ubuntu system without having to
    # answer _any_ questions interactively, except perhaps providing
    # your password when prompted by 'sudo'.
    # https://askubuntu.com/questions/1275842/install-wireshark-without-confirm
    echo "wireshark-common wireshark-common/install-setuid boolean true" | sudo debconf-set-selections
    sudo DEBIAN_FRONTEND=noninteractive apt-get -y install wireshark tshark

    sudo apt-get -y install python3-pip
fi
if [ "${ID}" = "fedora" ]
then
    sudo yum install -y python3-pip
fi

pip3 install distro

mkdir -p $SDE_INSTALL
cd $SDE
git clone --depth=1 https://github.com/p4lang/target-utils --recursive utils
git clone --depth=1 https://github.com/p4lang/target-syslibs --recursive syslibs
git clone --depth=1 https://github.com/p4lang/p4-dpdk-target --recursive p4-dpdk-target

# NOTE: The version of meson installed by running the install_dep.py
# program below on some OS versions is:

# Ubuntu 20.04 'sudo apt-get install meson' -> meson 0.53.2 ninja 1.10.0 'meson compile'? no
# Ubuntu 22.04 'sudo apt-get install meson' -> meson 0.61.2 ninja 1.10.1 'meson compile'? yes
# Fedora 34 'yum install meson' -> meson 0.62.1 ninja 1.10.2 'meson compile'? yes

# Without a version of meson that implements the 'compile'
# sub-command, the p4-dpdk-target install script will not work.  meson
# 0.61.1 and 0.61.2 appear to be new enough, but 0.53.2 is not.

sudo -E python3 p4-dpdk-target/tools/setup/install_dep.py
cd $SDE/p4-dpdk-target
git submodule update --init --recursive --force
./autogen.sh
./configure --prefix=$SDE_INSTALL
make -j4
make install
