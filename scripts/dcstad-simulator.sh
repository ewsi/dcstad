#!/bin/bash

DCSTAD_REPLY="/dev/null"
if [ -n "$REPLY_FD" ]; then
  DCSTAD_REPLY="/proc/self/fd/$REPLY_FD"
fi


function validate_environment {
  if [ "$PRIMARY_INTF" == "" ]; then
    echo "Primary interface name not provided"
    exit 1
  fi
  if [ "$DATACHAN_INTF_COUNT" == "" ]; then
    echo "Data channel interface count not provided"
    exit 1
  fi
  if [ "$DATACHAN_INTF_COUNT" == "0" ]; then
    echo "No data channel interfaces provided"
    exit 1
  fi
}

function validate_ssid_environment {
  if [ "$DATACHAN_SSID_COUNT" == "" ]; then
    echo "Data channel SSID count not provided"
    exit 1
  fi
  if [ "$DATACHAN_SSID_COUNT" == "0" ]; then
    echo "No data channel SSIDs provided"
    exit 1
  fi
}

function dcw_startup {
  echo "Simulating process startup..."
  validate_environment

  i=0
  while [ $i -lt $DATACHAN_INTF_COUNT ]; do
    dcintf_envname="DATACHAN_INTF_$i"
    dcintf="${!dcintf_envname}"
    if [ "$dcintf" == "" ]; then
      echo "Data channel interface name $i not provided"
      return 1
    fi
    echo "I was given a data channel Interface '$dcintf'"
    let i+=1
  done

  echo "Returing success (0) back to dcwstad"
  return 0
}

function dcw_shutdown {
  echo "Simulating process shutdown..."
  validate_environment

  i=0
  while [ $i -lt $DATACHAN_INTF_COUNT ]; do
    dcintf_envname="DATACHAN_INTF_$i"
    dcintf="${!dcintf_envname}"
    if [ "$dcintf" == "" ]; then
      echo "Data channel interface name $i not provided"
      return 1
    fi
    echo "I was given a data channel Interface '$dcintf'"
    let i+=1
  done

  echo "Returing success (0) back to dcwstad"
  return 0
}

function dcw_join {
  echo "Simulating a DCW join event..."
  validate_environment
  validate_ssid_environment

  i=0
  while [ $i -lt $DATACHAN_INTF_COUNT ]; do
    dcintf_envname="DATACHAN_INTF_$i"
    dcintf="${!dcintf_envname}"
    if [ "$dcintf" == "" ]; then
      echo "Data channel interface name $i not provided"
      return 1
    fi
    echo "I was given a data channel Interface '$dcintf'"
    echo "$dcintf $DATACHAN_SSID_1" >> $DCSTAD_REPLY
    let i+=1
  done

  i=0
  while [ $i -lt $DATACHAN_SSID_COUNT ]; do
    dcssid_envname="DATACHAN_SSID_$i"
    dcssid="${!dcssid_envname}"
    if [ "$dcssid" == "" ]; then
      echo "Data channel interface name $i not provided"
      return 1
    fi
    echo "I was given a data channel SSID '$dcssid'"
    let i+=1
  done

  echo "Returing success (0) back to dcwstad"
  return 0
}

function dcw_unjoin {
  echo "Simulating a DCW unjoin event..."
  validate_environment
  validate_ssid_environment

  ssid_idx=0
  while [ $ssid_idx -lt $DATACHAN_SSID_COUNT ]; do
    dcssid_envname="DATACHAN_SSID_$ssid_idx"
    dcssid="${!dcssid_envname}"
    if [ "$dcssid" == "" ]; then
      echo "Data channel SSID name $ssid_idx not provided"
      return 1
    fi

    dcintf_idx=0
    while [ $dcintf_idx -lt $DATACHAN_INTF_COUNT ]; do
      dcintf_envname="DATACHAN_INTF_$dcintf_idx"
      dcintf="${!dcintf_envname}"
      if [ "$dcintf" == "" ]; then
        echo "Data channel interface name $dcintf_idx not provided"
        return 1
      fi
      echo "Unjoining interface \"$dcintf\" from SSID \"$dcssid\""
      let dcintf_idx+=1
    done

    let ssid_idx+=1
  done

  echo "Returing success (0) back to dcwstad"
  return 0;
}


exec 1>&2 #remap all stdout to goto stderr

case "$REASON" in

  STARTUP)
    dcw_startup; exit $?
    ;;

  SHUTDOWN)
    dcw_shutdown; exit $?
    ;;

  JOIN)
    dcw_join; exit $?
    ;;

  UNJOIN)
    dcw_unjoin; exit $?
    ;;

  *)
    echo "Unhandled reason: \"$REASON\"... Bailing out successfully"
    exit 0
esac

