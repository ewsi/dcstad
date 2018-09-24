#ifndef EVENT_WAITER_H_INCLUDED
#define EVENT_WAITER_H_INCLUDED

#include <dcwproto.h>

#include "./dcstad_config.h"

typedef enum {
  EVENTTYPE_FAILURE     = 0x0,
  EVENTTYPE_INTERRUPT   = 0x1,
  EVENTTYPE_PHY         = 0x2,
  EVENTTYPE_MSGRECV     = 0x4,

} eventtype_t;

eventtype_t wait_for_event(dcstad_cfg_t /* cfg */, struct dcwmsg * const /* msg */);

#endif /* #ifndef EVENT_WAITER_H_INCLUDED */
