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


#ifndef SCRIPT_INVOKER_H_INCLUDED
#define SCRIPT_INVOKER_H_INCLUDED

#include "./dcstad_config.h"

typedef struct script_retbuf {
  char     * buf;
  unsigned   buf_size;
  unsigned   buf_maxsize;

} *script_retbuf_t;


typedef struct data_ssid_list {
  unsigned            count;
  dcwmsg_ssid_t       ssids[32];
} *data_ssid_list_t;

int invoke_dcstad_script(
  dcstad_cfg_t /* cfg */,
  const char * const /* reason */,
  const data_ssid_list_t /* data_ssids */,
  script_retbuf_t /* retbuf */
);

#endif /* #ifndef SCRIPT_INVOKER_H_INCLUDED */
