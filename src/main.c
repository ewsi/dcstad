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


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <signal.h>

#include "./dcwlog.h"

#include "./dcstad_config.h"
#include "./script_invoker.h"
#include "./signaling.h"
#include "./event_waiter.h"


volatile static int _run;

static void
interrupt_run(int signum) {
  _run = 0;
}

static int
run_loop(dcstad_cfg_t cfg) {
  int             joined;
  eventtype_t     eventtype;
  struct dcwmsg   eventmsg;

  joined = 0;
  _run = 1;
  signal(SIGINT,  &interrupt_run);
  signal(SIGQUIT, &interrupt_run);
  signal(SIGTERM, &interrupt_run);

  while (_run) {
    joined = 0;
    dcwlogdbgf("%s\n", "Attempting Join...");
    if (!attempt_join_transaction(cfg)) {
      sleep(5);
      continue;
    }
    joined = 1;

    dcwloginfof("%s\n", "Joined AP!");

    /* if we get here, then weve had a successful join... */
    while (_run && joined) {
      eventtype = wait_for_event(cfg, &eventmsg);
      if (eventtype == EVENTTYPE_FAILURE) {
        /* something wicked happened... bailout */
        dcwlogerrf("%s\n", "Something threw a wrench in the engine. Bailing...");
        return 1; /* XXX unfortunately cant send unjoin here because socket is most likely in bad state */
      }
      if (eventtype & EVENTTYPE_INTERRUPT) {
        dcwlogdbgf("%s\n", "Got an interrupt event...");
      }
      if (eventtype & EVENTTYPE_PHY) {
        dcwlogdbgf("%s\n", "Got a PHY event...");
        joined = 0;
      }
      if (eventtype & EVENTTYPE_MSGRECV) {
        /* got a message originating from the AP... */
        dcwlogdbgf("%s\n", "Got a message from the AP...");
        switch (eventmsg.id) {
        case DCWMSG_AP_QUIT:
          dcwloginfof("%s\n", "The AP just bailed on us");
          joined = 0;
          break;
        default:
          dcwlogwarnf("Ignoring a message from the AP: %d\n", (int)eventmsg.id);
          break;
        }
      }
    }

    /* unjoin */
    perform_ap_unjoin(cfg);
  }

  return 0; /* success */
}

static void
usage( void ) {
  fprintf(stderr, "Usage: -p<primary channel> -d<data channel> -s<script file name>\n");
  _exit(1);
}

static void
populate_channel_mac_address(dcstad_chan_t cfg) {
  dcw_socket_t s;

  if ((s = dcwsock_open(cfg->ifname)) == NULL) {
    dcwlogerrf("Failed to retrieve MAC address for interface '%s'\n", cfg->ifname);
    _exit(1);
  }
  dcwsock_get_macaddr(s, cfg->macaddr);
  dcwsock_close(s);
}

static void
parse_cmdline(dcstad_cfg_t cfg, int argc, char *argv[]) {
  int option;
  unsigned i;

  /* insure we got enough args */
  if (argc < 3) usage();
  
  /* for each cmd line arg... */
  while ((option=getopt(argc, argv, "p:d:s:")) != -1) {
    switch (option) {
    case 'p':
      strncpy(cfg->primchan.ifname, optarg, sizeof(cfg->primchan.ifname));
      break;

    case 'd':
      strncpy(cfg->datachan[cfg->datachan_count++].ifname, optarg, sizeof(cfg->datachan[0].ifname));
      break;

    case 's':
      strncpy(cfg->script_path, optarg, sizeof(cfg->script_path));
      break;

    default:
      usage();
    }
  }

  /* ensure the bare miminum parameters are supplied... */
  if (cfg->primchan.ifname[0] == '\0') usage();
  if (cfg->datachan_count < 1) usage();
  if (cfg->script_path[0] == '\0') usage();

  /* validate script file exist and is executable... */
  if (access(cfg->script_path, X_OK) != F_OK) {
    dcwlogerrf("Can't execute script file '%s'\n", cfg->script_path);
    _exit(1);
  }

  /* populate the interface MAC addresses */
  populate_channel_mac_address(&cfg->primchan);
  for (i = 0 ; i < cfg->datachan_count; i++) {
    populate_channel_mac_address(&cfg->datachan[i]);
  }

  /* open up the signaling (primary channel) socket */
  if ((cfg->signaling_sock = dcwsock_open(cfg->primchan.ifname)) == NULL) {
    /* this shouldnt of failed because populate_channel_mac_address() succeeded... */
    dcwlogerrf("%s\n", "Failed to open signaling socket...");
    _exit(1);
  }

  /* set the close-on-exec flag on the signalling sock so the "dcstad script" cant get to it */
  fcntl(dcwsock_get_fd(cfg->signaling_sock), F_SETFD, fcntl(dcwsock_get_fd(cfg->signaling_sock), F_GETFD) | FD_CLOEXEC);
}

int
main( int argc, char *argv[] ) {
  struct dcstad_cfg cfg;
  int rv;

  rv = 1; /* failure unless proven otherwise */

  /* first initialize and parse the command line */
  bzero(&cfg, sizeof(cfg));
  parse_cmdline(&cfg, argc, argv);

  dcwloginfof("%s\n", "DCW Station Daemon Starting Up...");

  /* invoke the "dcstad script" to inform it we are starting up */
  if (invoke_dcstad_script(&cfg, "STARTUP", NULL, NULL) != 0) {
    /* either failed to run the script or the script didnt like something about our setup... */
    dcwlogerrf("Execution of the DCSTAD script (\"%s\") on STARTUP failed.\n", cfg.script_path);
    goto done;
  }

  /* enter the run loop */
  rv = run_loop(&cfg);

done:
  dcwloginfof("%s\n", "DCW Station Daemon Shutting Down...");

  /* invoke the "dcstad script" to inform it we are shutting down */
  if (invoke_dcstad_script(&cfg, "SHUTDOWN", NULL, NULL) != 0) {
    dcwlogwarnf("Execution of the DCSTAD script (\"%s\") on SHUTDOWN failed.\n", cfg.script_path);
  }

  /* shutdown the signaling socket... */
  dcwsock_close(cfg.signaling_sock);
  return rv;
}


