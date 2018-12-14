#!/bin/bash
##
##
## Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
##                    and others.  All rights reserved.
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at:
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
## Created by Jon Dennis (j.dennis@cablelabs.com)
##
##

DCSTAD_REPLY="/dev/null"
if [ -n "$REPLY_FD" ]; then
  #brace yourself... this is ugly...
  #because wpa_supplicant dont daemonize correctly,
  #we must suffer through this...
  #
  #wpa_supplicant hangs on to all inherited FD's
  #when it daemonizes itself... this causes the
  #DCSTAD daemon to hang when reading the reply
  #back because the file handle is never closed
  #as it becomes owned by the wpa_supplicant process
  #
  #the workaround is to make the reply file descriptor
  #always FD #99, then always force close (99>&-) it
  #when executing wpa_supplicant
  #
  #because bash dont allow you to remap/redirect
  #file descriptor numbers contained within a 
  #variable, we need to use a couple evil eval's here...
  #
  #and yeah, it would be nice if you could set the
  #close-on-exec flag from bash but whaddya gonna do
  eval "exec 99>&$REPLY_FD" #duplicate the reply fd # into fd #99
  eval "exec $REPLY_FD>&-"  #close the original fd #
  #DCSTAD_REPLY="/proc/self/fd/$REPLY_FD"
  DCSTAD_REPLY="/proc/self/fd/99"
fi


function get_joined_ssid {
  wifname=$1

  #first get the SSID name...
  ssid=`iwgetid -r "$wifname"`
  if [ $? != 0 ]; then
    return 1
  fi

  if [ "$ssid" == "" ]; then
    return 1
  fi

  #then make sure it has a channel
  iwgetid -cr "$wifname" 2>&1 >/dev/null
  if [ $? != 0 ]; then
    return 1
  fi

  echo "$ssid"
  return 0
}

function find_wpa_supplicant_pid {
  wifname=$1
  pid=`pgrep -f 'wpa_supplicant.*-i\s*'"$wifname"`
  if [ $? != 0 ]; then
    return 1
  fi
  echo $pid
  return 0
}

function get_wpa_supplicant_conffile {
  pid=$1

  read -ra procargv <<<`cat /proc/$pid/cmdline 2>/dev/null | tr '\000' '\n'`

  next_arg_is_conffile=0
  for i in "${procargv[@]}"; do
    if [ $next_arg_is_conffile != 0 ]; then
      echo "$i"
      return 0
    fi

    if [ "${i:0:2}" != "-c" ]; then
      continue
    fi
    if [ "${i:2}" == "" ]; then
      next_arg_is_conffile=1
      continue
    fi
    echo "${i:2}"
    return 0
  done

  return 1
}

function get_wpa_supplicant_dcw_conffile_path {
  intfname=$1

  echo "/tmp/dcwwpa.$intfname".cfg
}

function generate_wpa_supplicant_datachan_conffile {
  
  pc_intfname=$1
  dc_intfname=$2
  dc_ssid=$3

  #this function got a bit tricky...
  #the high-level idea here is to generate
  #the data channel WPA configuration by making a copy
  #of the primary channel's WPA configuration, but
  #finding/replacing every instance of the primary SSID
  #string and changing it with the data SSID string

  sed_literal_escape='s/[]\/$*.^[]/\\&/g'

  pc_ssid=`get_joined_ssid "$pc_intfname"`
  if [ $? != 0 ]; then
    echo "Failed to generate WPA supplicant datachannel configuration due to the primary channel not being currently joined to an SSID"
    return 1
  fi

  pc_pid=`find_wpa_supplicant_pid "$pc_intfname"`
  if [ $? != 0 ]; then
    echo "Failed to generate WPA supplicant datachannel configuration due to the primary channel WPA supplicant process not running"
    return 1
  fi

  pc_conffile=`get_wpa_supplicant_conffile "$pc_pid"`
  if [ $? != 0 ]; then
    echo "Failed to generate WPA supplicant datachannel configuration due to the primary channel configuration not found"
    return 1
  fi

  replace_regex='s/"\?'`echo "$pc_ssid" | sed -e "$sed_literal_escape"`'"\?/"'`echo "$dc_ssid" | sed -e "$sed_literal_escape"`'"/g'

  cat $pc_conffile | grep -v '^ctrl_interface=' | grep -v '^update_config=' | sed -e "$replace_regex" > `get_wpa_supplicant_dcw_conffile_path "$dc_intfname"`
  return 0
}

function setup_datachan_intf {
  wifname=$1

  #blow away any wpa supplicant processes running on the given interface
  while [ 1 ]; do
    pid=`find_wpa_supplicant_pid "$wifname"`
    if [ $? != 0 ]; then
      break
    fi
    echo "Killing datachannel ($wifname) WPA supplicant pid# $pid"
    kill "$pid"
    sleep 1
  done

  #reset the interface wireless configuration...
  iwconfig "$wifname" mode managed
  iwconfig "$wifname" essid off

  #reset the interface ip configuration...
  ifconfig "$wifname" up 0.0.0.0

  #disable any reverse path filtering
  echo 0 > /proc/sys/net/ipv4/conf/all/rp_filter
  echo 0 > /proc/sys/net/ipv4/conf/"$wifname"/rp_filter

  #shut off ipv6
  echo 1 > /proc/sys/net/ipv6/conf/"$wifname"/disable_ipv6
}

function try_datachannel_join {
  pc_intfname=$1
  dc_intfname=$2
  dc_ssidname=$3
  maxwait_seconds=$4

  # first clear out any running state of the data channel interface...
  setup_datachan_intf "$dc_intfname"

  # determine if we need a WPA configuration...
  find_wpa_supplicant_pid "$pc_intfname" 2>&1 > /dev/null
  if [ $? == 0 ]; then
    #yup... WPA supplicant and config is needed...
    generate_wpa_supplicant_datachan_conffile "$pc_intfname" "$dc_intfname" "$dc_ssidname"
    if [ $? != 0 ]; then
      return 1
    fi

    #kick it off...
    #(Note: see the comment at the top of this file regarding the "99>&-")
    wpa_supplicant -B -i "$dc_intfname" -c `get_wpa_supplicant_dcw_conffile_path "$dc_intfname"` 99>&-
    if [ $? != 0 ]; then
      echo "Failed to kick-off WPA supplicant for data channel interface \"$dc_intfname\""
      setup_datachan_intf "$dc_intfname"
      return 1
    fi
  fi

  #set the SSID on the data channel adapter...
  iwconfig "$dc_intfname" essid "$dc_ssidname"
  if [ $? != 0 ]; then
    echo "Failed to set the SSID (\"$dc_ssidname\") for data channel interface \"$dc_intfname\""
    setup_datachan_intf "$dc_intfname"
    return 1
  fi

  echo "Waiting up to $maxwait_seconds seconds for interface \"$dc_intfname\" to join SSID \"$dc_ssidname\"..."

  #wait for it to join...
  while [ $maxwait_seconds -gt 0 ]; do
    get_joined_ssid "$dc_intfname" 2>&1 > /dev/null
    if [ $? == 0 ]; then
      echo "Successfully joined interface \"$dc_intfname\" on to SSID \"$dc_ssidname\""
      iwconfig "$dc_intfname" power off
      return 0 #join success
    fi

    #join failed... wait a sec and check again...
    sleep 1
    let maxwait_seconds-=1
  done

  #join failed... clear things out...
  echo "Timeout while attempting to join interface \"$dc_intfname\" on to SSID \"$dc_ssidname\""
  setup_datachan_intf "$dc_intfname"
  return 1
}

function validate_wintf {
  intf=$1

  iwconfig "$intf" 2>&1 >/dev/null
  if [ $? != 0 ]; then
    echo "Given interface (\"$intf\") does not appear to be a wireless adapter"
    exit 1
  fi
}


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


exec 1>&2 #remap all stdout to goto stderr

case "$REASON" in

  STARTUP)
    validate_environment
    validate_wintf "$PRIMARY_INTF"
    i=0
    while [ $i -lt $DATACHAN_INTF_COUNT ]; do
      dcintf_envname="DATACHAN_INTF_$i"
      dcintf="${!dcintf_envname}"
      if [ "$dcintf" == "" ]; then
        echo "Data channel interface name $i not provided"
        exit 1
      fi
      echo "Initializing data channel Interface '$dcintf'"
      setup_datachan_intf "$dcintf"
      if [ $? != 0 ]; then
        echo "Failed to setup data channel interface '$dcintf'"
        exit 1
      fi
      let i+=1
    done
    ;;

  SHUTDOWN)
    validate_environment
    i=0
    while [ $i -lt $DATACHAN_INTF_COUNT ]; do
      dcintf_envname="DATACHAN_INTF_$i"
      dcintf="${!dcintf_envname}"
      if [ "$dcintf" == "" ]; then
        echo "Data channel interface name $i not provided"
        exit 1
      fi
      echo "Clearing data channel Interface '$dcintf'"
      setup_datachan_intf "$dcintf"
      ifconfig "$dcintf" down
      let i+=1
    done
    ;;

  JOIN)
    validate_environment
    validate_ssid_environment
    validate_wintf "$PRIMARY_INTF"

    #XXX this could use some logic as to which interface gets which ssid
    join_success=0
    intf_idx=0
    while [ $intf_idx -lt $DATACHAN_INTF_COUNT ]; do
      dcintf_envname="DATACHAN_INTF_$intf_idx"
      dcintf="${!dcintf_envname}"
      if [ "$dcintf" == "" ]; then
        echo "Data channel interface name $intf_idx not provided"
        exit 1
      fi

      ssid_idx=0
      while [ $ssid_idx -lt $DATACHAN_SSID_COUNT ]; do
        dcssid_envname="DATACHAN_SSID_$ssid_idx"
        dcssid="${!dcssid_envname}"
        if [ "$dcssid" == "" ]; then
          echo "Data channel SSID name $ssid_idx not provided"
          return 1
        fi

        try_datachannel_join "$PRIMARY_INTF" "$dcintf" "$dcssid" 15
        if [ $? == 0 ]; then
          #we had a successful join... tell the daemon about what got bonded to what...
          echo "$dcintf $dcssid" >> $DCSTAD_REPLY
          join_success=1
          break;
        fi

        let ssid_idx+=1
      done
      let intf_idx+=1
    done

    if [ $join_success == 0 ]; then
      echo "Failed to join a single data channel interface to the AP"
      exit 1 #failed... not a single interface/SSID bonded...
    fi

    exit 0
    ;;

  UNJOIN)
    validate_environment
    i=0
    while [ $i -lt $DATACHAN_INTF_COUNT ]; do
      dcintf_envname="DATACHAN_INTF_$i"
      dcintf="${!dcintf_envname}"
      if [ "$dcintf" == "" ]; then
        echo "Data channel interface name $i not provided"
        exit 1
      fi
      echo "Clearing data channel Interface '$dcintf'"
      setup_datachan_intf "$dcintf"
      ifconfig "$dcintf" down
      let i+=1
    done
    ;;


  *)
    echo "Unhandled reason: \"$REASON\"... Bailing out successfully"
    exit 0
esac

