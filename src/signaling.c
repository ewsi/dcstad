

#include "./signaling.h"
#include "./dcw_transaction.h"
#include "./script_invoker.h"
#include "./dcwlog.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>


static void
invoke_script_join(dcstad_cfg_t cfg, struct dcwmsg * const msg) {
  char retbuf[10240];
  char linebuf[256];
  struct script_retbuf rb = {
    buf:          retbuf,
    buf_maxsize:  sizeof(retbuf),
  };
  struct data_ssid_list dsl;
  unsigned i;
  FILE *rbfp;
  char *ifname, *ssid;

  rbfp = NULL;

  /* populate the data ssid list from the message... */
  dsl.count = msg->ap_accept_sta.data_ssid_count;
  for (i = 0; i < dsl.count; i++) 
    memcpy(dsl.ssids[i], msg->ap_accept_sta.data_ssids[i], sizeof(dcwmsg_ssid_t));

  /* fire off the script */
  if (invoke_dcstad_script(cfg, "JOIN", &dsl, &rb) != 0) {
    /* script returned failure... NACK all DCW mac addresses... */
    dcwlogerrf("%s\n", "Script invocation failed... NACKing JOIN");
    goto failure;
  }

  /* prep ourselves for a happy ACK */
  msg->id = DCWMSG_STA_ACK;
  msg->sta_ack.bonded_data_channel_count = 0;

  /* process the script's reply... */
  if ((rbfp = fmemopen(rb.buf, rb.buf_size, "r")) == NULL) {
    /* shouldnt get here */
    dcwlogperrorf("%s", "fmemopen() failed");
    goto failure;
  }

  /* for each line the script gave us, convert to a bond */
  while (fgets(linebuf, sizeof(linebuf), rbfp) != NULL) {
    /*
      the first ' ' (space) deliminates the data channel interface name from the SSID string 
      each bond is line ('\n') deliminated
      format is:
        wlan0 some ssid name
        wlan1 some_other_ssid_name
        wlan2 yetAnother data channel
     */

    /* remove the newline if present */
    if (strstr(linebuf, "\n")) strstr(linebuf, "\n")[0] = '\0';

    /* setup "ifname" and "ssid" string pointers... */
    ifname = linebuf;
    if ((ssid = strstr(ifname, " ")) == NULL) {
      dcwlogerrf("%s\n", "Bad reply format from script... NACKing JOIN");
      goto failure;
    }
    (*ssid++) = '\0';

    /* cant be more bonds than data channel interfaces */
    if (msg->sta_ack.bonded_data_channel_count >= cfg->datachan_count) {
      dcwlogerrf("%s\n", "Script replied with more bonds than data channels!");
      goto failure;
    }

    /* defensive... */
    if (msg->sta_ack.bonded_data_channel_count >= (sizeof(msg->sta_ack.bonded_data_channels) / sizeof(msg->sta_ack.bonded_data_channels[0])) ) {
      dcwlogerrf("%s\n", "Script replied with excessive bonds");
      goto failure;
    }

    /* ensure the ifname is valid */
    for (i = 0; i < cfg->datachan_count; i++)
      if (strcmp(ifname, cfg->datachan[i].ifname) == 0) break;
    if (i >= cfg->datachan_count) {
      dcwlogerrf("Script replied with invalid bonded interface name: %s\n", ifname);
      goto failure;
    }

    /* copy the data channel mac address of the given interface into the new bond */
    memcpy(msg->sta_ack.bonded_data_channels[msg->sta_ack.bonded_data_channel_count].macaddr, cfg->datachan[i].macaddr, sizeof(cfg->datachan[i].macaddr));

    /* ensure the ssid is valid */
    for (i = 0; i < dsl.count; i++)
      if (strncmp(ssid, dsl.ssids[i], sizeof(dsl.ssids[i])) == 0) break;
    if (i >= dsl.count) {
      dcwlogerrf("Script replied with invalid bonded SSID: %s\n", ssid);
      goto failure;
    }

    /* copy the SSID into the new bond */
    memcpy(msg->sta_ack.bonded_data_channels[msg->sta_ack.bonded_data_channel_count].ssid, dsl.ssids[i], sizeof(dcwmsg_ssid_t));

    /* increment the bonded channel count... */
    ++msg->sta_ack.bonded_data_channel_count;
  }

  /* make sure we have at least one bond... */
  if (msg->sta_ack.bonded_data_channel_count == 0) {
    /* no bonded data channels? thats a failure */
    dcwlogerrf("%s\n", "Script gave us no bonded data channels... NACKing JOIN");
    goto failure;
  }

  /* all is good... msg is an ACK */
  if (rbfp != NULL) fclose(rbfp);
  return;

failure:
  /* something didnt quite work out... form a NACK for all data channel MAC addresses */
  msg->id = DCWMSG_STA_NACK;
  msg->sta_nack.data_macaddr_count = cfg->datachan_count;
  for (i = 0; i < cfg->datachan_count; i++) {
    memcpy(msg->sta_nack.data_macaddrs[i], cfg->datachan[i].macaddr, sizeof(cfg->datachan[i].macaddr));
  }

  if (rbfp != NULL) fclose(rbfp);
  return;
}

int
attempt_join_transaction(dcstad_cfg_t cfg) {
  struct dcwmsg msg;
  unsigned i;
  const enum dcwmsg_id reply_filter[] = {
    DCWMSG_AP_ACCEPT_STA,
    DCWMSG_AP_REJECT_STA,
    0
  };

  /* set the AP mac address as broadcast to discover it */
  memset(cfg->ap_macaddr, 0xFF, sizeof(cfg->ap_macaddr));

  /* set the message type to join */
  msg.id = DCWMSG_STA_JOIN;

  /* add our data channel MAC addresses to the message */
  msg.sta_join.data_macaddr_count = cfg->datachan_count;
  for (i = 0; i < cfg->datachan_count; i++) {
    memcpy(msg.sta_join.data_macaddrs[i], cfg->datachan[i].macaddr, sizeof(cfg->datachan[i].macaddr));
  }

  /* try to send our message, then wait for a reply... */
  dcwlogdbgf("%s\n", "Broadcasting JOIN");
  if (perform_dcw_transaction(cfg->signaling_sock, &msg, cfg->ap_macaddr, 5, reply_filter) != 1) {
    dcwlogdbgf("%s\n", "perform_dcw_transaction() failed");
    return 0; /* failed */
  }

  /* got a reply... */
  dcwlogdbgf("Reply received from: %02X-%02X-%02X-%02X-%02X-%02X\n",
    (unsigned)cfg->ap_macaddr[0],
    (unsigned)cfg->ap_macaddr[1],
    (unsigned)cfg->ap_macaddr[2],
    (unsigned)cfg->ap_macaddr[3],
    (unsigned)cfg->ap_macaddr[4],
    (unsigned)cfg->ap_macaddr[5]
  );

  /* validate response... */
  if (msg.id != DCWMSG_AP_ACCEPT_STA) {
    dcwlogerrf("%s\n", "AP did not accept us!");
    return 0; /* failed */
  }
  if (msg.ap_accept_sta.data_ssid_count < 1) {
    dcwlogerrf("%s\n", "AP did not give us any SSIDs!");
    return 0; /* failed */
  }

  /* invoke the script join reason... */
  invoke_script_join(cfg, &msg);

  /* send a few replies... (ACK || NACK) */
  for (i = 0; i < 3; i++) {
    dcwlogdbgf("Transmitting %s reply\n", (msg.id == DCWMSG_STA_ACK) ? "ACK" : "NACK");
    if (!dcwsock_send_msg(cfg->signaling_sock, &msg, cfg->ap_macaddr)) {
      dcwlogdbgf("%s\n", "dcw_send_msg() failed");
      return 0; /* failed */
    }
    usleep(250000);
  }

  return (msg.id == DCWMSG_STA_ACK) ? 1 : 0;
}


void
perform_ap_unjoin(dcstad_cfg_t cfg) {
  struct dcwmsg msg;
  unsigned i;
  unsigned retry_cnt;
  const unsigned max_retries = 3;
  const enum dcwmsg_id reply_filter[] = {
    DCWMSG_AP_ACK_DISCONNECT,
    0
  };

  /* try for a set amount of times for an ACK from the AP to our UNJOIN request */
  for (retry_cnt = 0; retry_cnt < max_retries; ++retry_cnt) {
    /* set the message type to unjoin */
    msg.id = DCWMSG_STA_UNJOIN;

    /* add our data channel MAC addresses to the message */
    msg.sta_unjoin.data_macaddr_count = cfg->datachan_count;
    for (i = 0; i < cfg->datachan_count; i++) {
      memcpy(msg.sta_unjoin.data_macaddrs[i], cfg->datachan[i].macaddr, sizeof(cfg->datachan[i].macaddr));
    }

    /* try to send our message, then wait for a reply... */
    dcwlogdbgf("%s\n", "Transmitting UNJOIN");
    if (perform_dcw_transaction(cfg->signaling_sock, &msg, cfg->ap_macaddr, 1, reply_filter) != 1) {
      continue;
    }
    dcwloginfof("%s\n", "AP Acknowledged UNJOIN");
    break;
  }

  /* if AP never ACK'd, print a warning and move on */
  if (retry_cnt >= max_retries)
    dcwlogwarnf("%s\n", "AP did NOT Acknowledge UNJOIN");

  /* invoke the script unjoin reason... */
  if (invoke_dcstad_script(cfg, "UNJOIN", NULL, NULL) != 0) {
    dcwlogwarnf("%s\n", "Script did not successfully UNJOIN");
  }
}
