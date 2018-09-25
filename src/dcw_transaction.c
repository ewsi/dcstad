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





#include "./dcw_transaction.h"

#include "./dcwlog.h"

#include <stdio.h>
#include <string.h>
#include <sys/select.h>

int
dcwsock_recv_msg(
  dcw_socket_t                   s,
  struct dcwmsg * const          msg,
  unsigned char * const          endpoint
  ) {
  unsigned char buf[1500];
  int rv;

  rv = dcwsock_recv(s, buf, sizeof(buf), endpoint);
  if (rv <= -1) {
    dcwlogerrf("%s\n", "dcwsock_recv() failed");
    return -1; /* socket is probably in a bad state */
  }
  if (rv == 0) {
    dcwlogerrf("%s\n", "dcwsock_recv() empty/bad incoming message");
    return 0; /* empty/bad incoming message */
  }

  /* got buffer... try to marshal it... */
  if (!dcwmsg_marshal(msg, buf, rv)) {
    /* marshal failed */
    dcwlogerrf("%s\n", "dcwmsg_marshal() failed");
    return 0; /* empty/bad incoming message */
  }

  return 1; /* success */
}

static int
dcwsock_filtered_recv_timeout(
  dcw_socket_t                   s,
  struct dcwmsg * const          msg,
  unsigned char * const          endpoint,
  unsigned                       timeout,
  const enum dcwmsg_id * const   msg_type_filter
  ) {

  int     rv;
  int     fd;
  fd_set  readfds;
  struct  timeval tv;
  const enum dcwmsg_id *filter_ptr;
  unsigned char from[6];

  static unsigned char bcast_macaddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  fd         = dcwsock_get_fd(s);
  tv.tv_sec  = timeout;
  tv.tv_usec = 0;

  while ( 1 ) {
    /* wait for up to timeout (or indefinately if timeout is zero) for a socket message */
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    rv = select(dcwsock_get_fd(s) + 1, &readfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
    if (rv == 0) {
      /* timeout */
      return 0;
    }
    if (rv == -1) {
      /* failure */
      dcwlogperrorf("%s", "select() failed in filtered recv");
      return -1;
    }
    if (!FD_ISSET(fd, &readfds)) continue; /* defensive; in-theory this shouldn't happen... */

    /* a frame is ready to be read... */
    switch (dcwsock_recv_msg(s, msg, from)) {
    case -1:
      return -1; /* socket is probably in a bad state... bailout here... */
    case 0:
      continue; /* bad recv, but keep going until we reach timeout... */
    }

    /* successfully got an incoming DCW message */
    /* do we have a message type filter? */
    if (msg_type_filter != NULL) {
      for (filter_ptr = msg_type_filter; (*filter_ptr) != 0; filter_ptr++) {
        if ((*filter_ptr) == msg->id) break;
      }
      if ((*filter_ptr) == 0) continue; /* filter does not match... try again... */
      /* if we get here the incoming messages matches a type/id in the provided filter */
    }

    /* message type/id filter on the incoming message checks out...
       do we need to filter on the from mac address? */
    if (endpoint != NULL) {
      /* the logic here is:
         . if "endpoint" is broadcast, then replace it with the first response
           back matching the message type/id
         . if "endpoint" is something else, then it must match the sender mac
           address of the response...
      */
      if (memcmp(endpoint, bcast_macaddr, sizeof(bcast_macaddr)) != 0) {
        /* we were given a non-broadcast endpoint... ensure it matches the sender... */
        if (memcmp(from, endpoint, sizeof(from)) != 0) {
          /* we picked up a DCW message, but it was not from who 
             we expected it to be from... try again... */
          continue;
        }
      }
      /* replace the caller's endpoint buffer with the sender's MAC address */
      memcpy(endpoint, from, sizeof(from));
    }

    return 1; /* success */
  }

  return -1; /* defensive, shouldnt get here */
}

int
dcwsock_send_msg(
  dcw_socket_t                   s,
  const struct dcwmsg * const    msg,
  const unsigned char * const    endpoint
  ) {

  unsigned char buf[1500];
  unsigned buf_len;
  int rv;

  /* serialize the message to transmit... */
  buf_len = dcwmsg_serialize(buf, msg, sizeof(buf));
  if (buf_len == 0) {
    dcwlogerrf("%s\n", "dcwmsg_serialize() failed");
    return 0; /* failure */
  }

  /* belt the serialized message out onto the wire... */
  rv = dcwsock_send(s, buf, buf_len, endpoint);
  if (rv != buf_len) {
    dcwlogerrf("%s\n", "dcwsock_send() failed");
    return 0; /* failure */
  }

  return 1; /* success */
}

int
perform_dcw_transaction(
  dcw_socket_t                   s,
  struct dcwmsg * const          msg,
  unsigned char * const          endpoint,
  const unsigned                 reply_timeout,
  const enum dcwmsg_id * const   msg_type_filter
  ) {

  /* attempt to serialize and send out the caller's message */
  if (!dcwsock_send_msg(s, msg, endpoint)) {
    return -1; /* failure */
  }

  /* does the caller expect us to receive something as well? */
  if (reply_timeout == 0) {
    return 1; /* success */
  }

  /* try to receive a message */
  return dcwsock_filtered_recv_timeout(s, msg, endpoint, reply_timeout, msg_type_filter);
}


