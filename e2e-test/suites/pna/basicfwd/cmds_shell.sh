#!/usr/bin/env bash

# Set up TAP ports
for port in {0..3}; do
    sudo ip link set "TAP$port" up
done

# Disable IPv6 because it can interfere with packet sniffing
sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1
sudo sysctl -w net.ipv6.conf.default.disable_ipv6=1
sudo sysctl -w net.ipv6.conf.lo.disable_ipv6=1
