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



#include "./dcwlog.h"

#include "./event_waiter.h"
#include "./dcw_transaction.h"

#include <dcwsocket.h>

#include <sys/select.h>
#include <errno.h>
#include <string.h>



eventtype_t
wait_for_event(dcstad_cfg_t cfg, struct dcwmsg * const msg) {
  fd_set          rfds;
  int             sockfd;
  eventtype_t     rv;
  unsigned char   msgfrom[6];

  sockfd = dcwsock_get_fd(cfg->signaling_sock);

  while ( 1 ) {
    /* clear out the file set */
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    /* wait for something to happen... */
    dcwlogdbgf("%s\n", "select()ing for events...");
    if (select(sockfd + 1, &rfds, NULL, NULL, NULL) == -1) {
      dcwlogdbgf("%s\n", "done select()ing for events (failed)...");
      if (errno == EINTR) {
        return EVENTTYPE_INTERRUPT;
      }
      dcwlogperrorf("%s", "select() failed for some reason");
      return EVENTTYPE_FAILURE;
    }
    dcwlogdbgf("%s\n", "done select()ing for events...");

    /* reset the event bitmap (return value) to zero */
    rv = 0;

    /* did the signalling socket (primary channel) trigger us ? */
    if (FD_ISSET(sockfd, &rfds)) {
      switch(dcwsock_recv_msg(cfg->signaling_sock, msg, msgfrom)) {
      case -1:
        return EVENTTYPE_FAILURE; /* socket is probably in a bad state... bailout here... */
      case 0:
        break; /* bad recv; ignore */
      default:
        /* "msg" contains a valid DCW packet... */
        if (memcmp(msgfrom, cfg->ap_macaddr, sizeof(msgfrom)) == 0) {
          /* we only care about traffic originating from the AP */
          rv |= EVENTTYPE_MSGRECV;
        }
        break;
      }
    }

    if (rv == 0) continue; /* select() worked, but there is nothing to do... try again... */
    return rv;
  }

  return EVENTTYPE_FAILURE; /* should never get here... */
}


