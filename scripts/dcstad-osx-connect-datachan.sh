#!/bin/bash


WIFI_PASSWORD="ABCDEFGH"

LOCATION_NAME="Dual Channel Wi-Fi"
DC_SERVICE_NAME="Data Channel"


function delete_all_services_except_datachannel {
  device="$1"
  networksetup -listnetworkserviceorder | grep -B1 "Device: $device" | grep '^(\d\d*)' | sed 's/([[:digit:]]*) //g' | \
  while read -r svcname; do
    if [ "$svcname" != "$DC_SERVICE_NAME" ]; then
      networksetup -removenetworkservice "$svcname"
    fi
  done
}

function provision_network_configuration {


  networksetup -switchtolocation "Automatic"
  networksetup -deletelocation "$LOCATION_NAME" > /dev/null 2>&1
  networksetup -createlocation "$LOCATION_NAME" populate
  networksetup -switchtolocation "$LOCATION_NAME"
  sleep 2 # without this sleep the network service never gets created for some reason...

  networksetup -removenetworkservice "$DC_SERVICE_NAME" > /dev/null 2>&1
  networksetup -createnetworkservice "$DC_SERVICE_NAME" "$DATACHAN_INTF_0"
  networksetup -setmanual "$DC_SERVICE_NAME" 169.254.0.255 255.255.255.255
  networksetup -setv6LinkLocal "$DC_SERVICE_NAME"
  networksetup -setnetworkserviceenabled "$DC_SERVICE_NAME" on
  delete_all_services_except_datachannel "$DATACHAN_INTF_0"

  networksetup -removeallpreferredwirelessnetworks "$DATACHAN_INTF_0" > /dev/null 2>&1
  networksetup -setairportpower "$DATACHAN_INTF_0" off
}

function restore_network_configuration {

  #services are per-location so this is not necessary...
  #networksetup -switchtolocation "$LOCATION_NAME"
  #networksetup -removenetworkservice "$DC_SERVICE_NAME" 

  networksetup -switchtolocation "Automatic"
  networksetup -deletelocation "$LOCATION_NAME"

  networksetup -removeallpreferredwirelessnetworks "$DATACHAN_INTF_0"
  networksetup -setairportpower "$DATACHAN_INTF_0" off
}


function try_datachannel_join {
  dc_intfname="$1"
  dc_ssidname="$2"

  networksetup -setairportpower "$dc_intfname" off
  networksetup -removeallpreferredwirelessnetworks "$dc_intfname" > /dev/null 2>&1
  networksetup -setairportpower "$dc_intfname" on
  networksetup -setairportnetwork "$dc_intfname" "$dc_ssidname" "$WIFI_PASSWORD"

  if [ "`networksetup -getairportnetwork $dc_intfname | sed 's/Current Wi-Fi Network: //'`" != "$dc_ssidname" ]; then
    echo joinfailed
    return 1
  fi

  echo dcsuccess
  return 0
}

function datachannel_unjoin {
  dc_intfname="$1"

  networksetup -removeallpreferredwirelessnetworks "$dc_intfname" > /dev/null 2>&1
  networksetup -setairportpower "$dc_intfname" off
}


function validate_environment {
  if [ "$DATACHAN_INTF_COUNT" == "" ]; then
    echo "Data channel interface count not provided"
    exit 1
  fi
  if [ "$DATACHAN_INTF_COUNT" != "1" ]; then
    echo "Only one data channel is supported with this script."
    exit 1
  fi
}

function validate_wifi_interface {
  intf="$1"
  networksetup -getairportnetwork "$intf" > /dev/null 2>&1
  if [ $? != 0 ]; then
    echo "Interface '$intf' does not appear to be a Wi-Fi interface. Currently, only built-in Apple Wi-Fi cards may be used as the data-channel."
    exit 1
  fi
}

exec 1>&2 #remap all stdout to goto stderr
if [ -n "$REPLY_FD" ]; then
  eval "exec 99>&$REPLY_FD" #remap FD 99 as the reply descriptor
fi

case "$REASON" in

  STARTUP)
    validate_environment
    validate_wifi_interface "$DATACHAN_INTF_0"

    provision_network_configuration
    exit 0
  ;;

  SHUTDOWN)
    restore_network_configuration
    exit 0
  ;;

  JOIN)
    validate_environment
    validate_wifi_interface "$DATACHAN_INTF_0"

    ssid_idx=0
    while [ $ssid_idx -lt $DATACHAN_SSID_COUNT ]; do
      dcssid_envname="DATACHAN_SSID_$ssid_idx"
      dcssid="${!dcssid_envname}"
      if [ "$dcssid" == "" ]; then
        echo "Data channel SSID name $ssid_idx not provided"
        return 1
      fi

      try_datachannel_join "$DATACHAN_INTF_0" "$dcssid"
      if [ $? == 0 ]; then
        #data channel join was successful... 
        #inform the daemon of the join
        echo "$DATACHAN_INTF_0 $dcssid" >&99
        exit 0 
      fi
      let ssid_idx+=1
    done

    echo "Data channel failed to join"
    exit 1
  ;;

  UNJOIN)
    validate_environment
    validate_wifi_interface "$DATACHAN_INTF_0"

    datachannel_unjoin "$DATACHAN_INTF_0"
    exit 0
  ;;

esac


