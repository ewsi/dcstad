#!/bin/bash

current_dir=$(dirname "$0")

# NOTE - The "REASON" is set as an environment variable before this script is invoked
#reason=$REASON
#echo "*** rpi-dcjoin.sh, REASON: $reason"

#export REASON="STARTUP"

export PRIMARY_INTF="wlan0"

export DATACHAN_INTF_COUNT="1"
export DATACHAN_INTF_0="wlan1"

export DATACHAN_SSID_COUNT="1"
export DATACHAN_SSID_0="OpenWrt-DCW"

exec ${current_dir}/dcstad-rpi.sh
#exit 0
