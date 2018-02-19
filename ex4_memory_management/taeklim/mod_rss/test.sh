#!/bin/bash

make
sudo rmmod process_rss_mod
sudo insmod process_rss_mod.ko
dmesg
