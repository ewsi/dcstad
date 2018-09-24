#!/bin/bash

export REASON="STARTUP"

export PRIMARY_INTF="wlan1"

export DATACHAN_INTF_COUNT="7"
export DATACHAN_INTF_0="wlan0"
export DATACHAN_INTF_1="wlan1"
export DATACHAN_INTF_2="wlan2"
export DATACHAN_INTF_3="wlan3"
export DATACHAN_INTF_4="wlan4"
export DATACHAN_INTF_5="wlan5"
export DATACHAN_INTF_6="wlan6"

export DATACHAN_SSID_COUNT="3"
export DATACHAN_SSID_0="datachan_ssid_1"
export DATACHAN_SSID_1="datachan_ssid_2"
export DATACHAN_SSID_2="datachan_ssid_3"

exec ./dcstad-simulator.sh


