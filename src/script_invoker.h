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
