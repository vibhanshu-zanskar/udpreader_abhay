#!/bin/bash

# ONLY APPLICABLE FOR ESTEE Server
# We are recieving packets on np1, and we are setting the params to rx fieldn max value.
ethtool -g enp1s0f1np1

sudo ethtool -G enp1s0f1np1 rx 8192

cat /proc/sys/vm/nr_hugepages

sudo echo 1024 > /proc/sys/vm/nr_hugepages
