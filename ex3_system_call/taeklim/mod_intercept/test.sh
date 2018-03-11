#!/bin/bash

make
sudo rmmod intercept_syscall
sudo insmod intercept_syscall.ko
dmesg
