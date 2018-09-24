
#include "./script_invoker.h"

#include "./dcwlog.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

static void 
stringify_mac_addr(char * const output, unsigned char * const macaddr, const unsigned output_size) {
  snprintf(output, output_size,
    "%02X:%02X:%02X:%02X:%02X:%02X",
    (unsigned)macaddr[0],
    (unsigned)macaddr[1],
    (unsigned)macaddr[2],
    (unsigned)macaddr[3],
    (unsigned)macaddr[4],
    (unsigned)macaddr[5]
  );
}

static void
setup_child_environment(
  dcstad_cfg_t cfg,
  const char * const reason,
  const data_ssid_list_t data_ssids,
  const int reply_fd
) {
  unsigned i;
  char buf[128];
  char mastr[20];

  /* environmentify the reason */
  setenv("REASON", reason, 1);

  /* environmentify the reply fd if provided */
  if (reply_fd != -1) {
    snprintf(buf, sizeof(buf), "%d", reply_fd);
    setenv("REPLY_FD", buf, 1);
  }

  /* environmentify the primary channel interface */
  setenv("PRIMARY_INTF", cfg->primchan.ifname, 1);
  stringify_mac_addr(mastr, cfg->primchan.macaddr, sizeof(mastr));
  setenv("PRIMARY_INTF_MACADDR", mastr, 1);

  /* environmentify the data channel interfaces */
  snprintf(buf, sizeof(buf), "%u", cfg->datachan_count);
  setenv("DATACHAN_INTF_COUNT", buf, 1);
  for (i = 0; i < cfg->datachan_count; i++) {
#warning ssid may not be null terminated!
    snprintf(buf, sizeof(buf), "DATACHAN_INTF_%u", i);
    setenv(buf, cfg->datachan[i].ifname, 1);
    stringify_mac_addr(mastr, cfg->datachan[i].macaddr, sizeof(mastr));
    strcat(buf, "_MACADDR");
    setenv(buf, mastr, 1);
  }

  /* environmentify the data channel SSIDs if provided */
  if (data_ssids != NULL) {
    snprintf(buf, sizeof(buf), "%u", data_ssids->count);
    setenv("DATACHAN_SSID_COUNT", buf, 1);
    for (i = 0; i < data_ssids->count; i++) {
      snprintf(buf, sizeof(buf), "DATACHAN_SSID_%u", i);
      setenv(buf, data_ssids->ssids[i], 1);
    }
  }
}

static void
ingest_retbuf(script_retbuf_t retbuf, const int fd) {
  ssize_t rv;

  if (retbuf == NULL) return;

  for (
    retbuf->buf_size = 0;
    (retbuf->buf_size < retbuf->buf_maxsize) && ((rv = read(fd, &retbuf->buf[retbuf->buf_size], retbuf->buf_maxsize)) > 0);
    retbuf->buf_size += rv);
}

int
invoke_dcstad_script(
  dcstad_cfg_t cfg,
  const char * const reason,
  const data_ssid_list_t data_ssids,
  script_retbuf_t retbuf
) {

  pid_t   script_pid;
  int     exit_status;
  int     rv;
  int     reply_pipe[2]; /* idx 0 = read from; idx 1 = write to */
  char    *argv[2];

  rv = -1; /* fail unless otherwise stated */
  reply_pipe[0] = reply_pipe[1] = -1;

  /* are we expecting a return buffer? if so, setup a pipe */
  if (retbuf != NULL) {
    if (pipe(reply_pipe) == -1) {
      dcwlogperrorf("%s", "pipe() failed");
      goto done;
    }
  }

  /* fork() off... */
  switch (script_pid = fork()) {
  case -1:
    /* uh-oh... */
    dcwlogperrorf("%s", "fork() failed");
    goto done;

  case 0:
    /* we're the child... */
    close(close(reply_pipe[0]));
    setup_child_environment(cfg, reason, data_ssids, reply_pipe[1]);

    /* exec the child... */
    argv[0] = cfg->script_path;
    argv[1] = NULL;
    execvp(argv[0], argv);

    /* only get here iv execvp() failed... */
    dcwlogperrorf("%s", "Child exec() failed");
    exit(-1);

  default:
    /* we're the parent... */
    dcwlogdbgf("Spawned process %d for script '%s'\n", (int)script_pid, cfg->script_path);

    /* ingest the reply if requested... */
    close(reply_pipe[1]);
    ingest_retbuf(retbuf, reply_pipe[0]);
    close(reply_pipe[0]);

    /* wait for the child to exit... */
    if (waitpid(script_pid, &exit_status, 0) == -1) return -1;
    rv = WIFEXITED(exit_status) ? WEXITSTATUS(exit_status) : -1;
    break;
  }

done:
  close(reply_pipe[0]);
  close(reply_pipe[1]);

  return rv;
}



