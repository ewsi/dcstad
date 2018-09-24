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
