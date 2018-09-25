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



#ifndef DCWLOG_H_INCLUDED
#define DCWLOG_H_INCLUDED


#include <stdio.h>


/* #define ENABLE_DCWDBG */

#define dcwloginfof(FMT, ...) fprintf(stderr, "[DCWINFO] " FMT, __VA_ARGS__)
#define dcwlogwarnf(FMT, ...) fprintf(stderr, "[DCWWARN] " FMT, __VA_ARGS__)
#define dcwlogerrf(FMT, ...)  fprintf(stderr, "[DCWERR] " FMT, __VA_ARGS__)

#ifdef ENABLE_DCWDBG
  #define dcwlogdbgf(FMT, ...)  fprintf(stderr, "[DCWDBG] " FMT, __VA_ARGS__)
#else
  #define dcwlogdbgf(FMT, ...) 
#endif

#define dcwlogperrorf(FMT, ...) {dcwlogerrf(FMT, __VA_ARGS__); perror("");}

#endif
