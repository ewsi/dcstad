/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/



#ifndef DCSTAD_CONFIG_H_INCLUDED
#define DCSTAD_CONFIG_H_INCLUDED

#include <dcwsocket.h>
#include <dcwproto.h>


typedef char           ifname_t[32];



typedef struct dcstad_chan {
  ifname_t           ifname;
  dcwmsg_macaddr_t   macaddr;
} * dcstad_chan_t;

typedef struct dcstad_cfg {
  dcw_socket_t        signaling_sock; /* AKA primary channel socket */
  struct dcstad_chan  primchan;

  unsigned            datachan_count;
  struct dcstad_chan  datachan[32];

  dcwmsg_macaddr_t    ap_macaddr;

  char                script_path[512];
} * dcstad_cfg_t;


#endif /* #ifndef DCSTAD_CONFIG_H_INCLUDED */
