#!/bin/bash

BASEDIR=$(pwd -P)

###
### program build
###
mkdir -p build
pushd build
cmake ..
make
popd

### 
### Setup
### 
prev_rdpmc=$(cat /sys/bus/event_source/devices/cpu/rdpmc)
echo 2 > /sys/bus/event_source/devices/cpu/rdpmc || exit

modprobe --first-time msr &>/dev/null
msr_prev_loaded=$?

# (Temporarily) disable watchdogs, see https://github.com/obilaniu/libpfc
! modprobe --first-time -r iTCO_wdt &>/dev/null
iTCO_wdt_prev_loaded=$?

! modprobe --first-time -r iTCO_vendor_support &>/dev/null
iTCO_vendor_support_prev_loaded=$?

prev_nmi_watchdog=$(cat /proc/sys/kernel/nmi_watchdog)
echo 0 > /proc/sys/kernel/nmi_watchdog


### 
### run
### 
echo "***** Start evaluation *****"
echo ">>>> check transient instructions are cached into uop cache (run transient_check) <<<<"
./build/transient_check

echo ">>>> check never reached instructions are NOT cached into uop cache (run transient_check_never_reach) <<<<"
./build/transient_check_never_reach


### 
### Restore
### 
echo
echo
echo
echo $prev_rdpmc > /sys/bus/event_source/devices/cpu/rdpmc
echo $prev_nmi_watchdog > /proc/sys/kernel/nmi_watchdog

if [[ $msr_prev_loaded == 0 ]]; then
    modprobe -r msr
fi

if [[ $iTCO_wdt_prev_loaded != 0 ]]; then
    modprobe iTCO_wdt &>/dev/null
fi

if [[ $iTCO_vendor_support_prev_loaded != 0 ]]; then
    modprobe iTCO_vendor_support &>/dev/null
fi
