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
