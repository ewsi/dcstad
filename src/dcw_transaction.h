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



#ifndef DCW_TRANSACTION_H_INCLUDED
#define DCW_TRANSACTION_H_INCLUDED

#include <dcwsocket.h>
#include <dcwproto.h>

/* 
dcwsock_recv_msg():

  The purpose of this function is to have a "direct to struct dcwmsg" recv function
  which takes care of both recving and marshaling a dsw msg in one shot

  Parameters:
    . s               -- open DCW socket handle
    . msg             -- msg struct to recv into
    . endpoint        -- the network endpoint (MAC address) the msg was recved from

  Return Values:
    . -1 -- DCW socket is probably in a bad state
    .  0 -- empty/bad incoming message
    .  1 -- success
*/
int dcwsock_recv_msg(dcw_socket_t /* s */, struct dcwmsg * const /* msg */, unsigned char * const /* endpoint */);

/* 
dcwsock_send_msg():

  The purpose of this function is to have a "direct from struct dcwmsg" send function
  which takes care of both serializing and sending a dsw msg in one shot

  Parameters:
    . s               -- open DCW socket handle
    . msg             -- msg struct to send
    . endpoint        -- the network endpoint (MAC address) to send the msg to

  Return Values:
    .  0 -- failure
    .  1 -- success
*/
int dcwsock_send_msg(dcw_socket_t /* s */, const struct dcwmsg * const /* msg */, const unsigned char * const /* endpoint */);

/* 
perform_dcw_transaction():

  The purpose of this function is to wrap up DCW protocol query/response transactions.
  A transaction here is defined as a request/response set.

  Parameters:
    . s               -- open DCW socket handle
    . msg             -- msg struct to send/recv
    . endpoint        -- expected network endpoint (MAC address) to send to and recv from
      . if the endpoint if broadcast MAC address, this will be re-populated with the
        MAC address of the first reply matching the filter
      . if the endpoint if not a broadcast MAC address, any replies not matching this
        MAC address will be filtered out
      . if this is NULL, then this field is ignored all toghether
    . reply_timeout   -- amount of time to wait for a reply; if 0, then we will skip recv'ing a reply
    . msg_type_filter -- pointer to array of message types/id we want to filter on
      . if this is NULL then no filtering will be applied on incoming messages

  Return Values:
    . -1 -- DCW socket is probably in a bad state
    .  0 -- timeout while waiting for a reply
    .  1 -- success

*/
int perform_dcw_transaction(dcw_socket_t /* s */, struct dcwmsg * const /* msg */, unsigned char * const /* endpoint */, const unsigned /* reply_timeout */, const enum dcwmsg_id * const /* msg_type_filter */);

#endif /* #ifndef DCW_TRANSACTION_H_INCLUDED */
