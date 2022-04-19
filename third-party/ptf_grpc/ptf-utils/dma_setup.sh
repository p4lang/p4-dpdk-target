#!/bin/bash

# Set up hugepages used for DMA buffer allocation
ID=$(id -u)
if [ $ID != 0 ]; then
  echo "ERROR: Run this script as root or with sudo"
  exit 1
else
  sh -c 'echo "#Enable huge pages support for DMA purposes" >> /etc/sysctl.conf'
  sh -c 'echo "vm.nr_hugepages = 128" >> /etc/sysctl.conf'
  sysctl -p /etc/sysctl.conf
  mkdir /mnt/huge
  mount -t hugetlbfs nodev /mnt/huge
fi
