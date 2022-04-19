/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
    bf_switchd_main.c
*/

/* Standard includes */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

/* Local includes */
#include "bf_switchd.h"
#include "switch_config.h"

static void bf_switchd_parse_hld_mgrs_list(bf_switchd_context_t *ctx,
                                           char *mgrs_list) {
  int len = strlen(mgrs_list);
  int i = 0;
  char mgr;

  while (i < len) {
    mgr = mgrs_list[i];
    switch (mgr) {
      case 'p':
        ctx->skip_hld.pipe_mgr = true;
        break;
      case 'r':
        ctx->skip_hld.port_mgr = true;
        break;
      default:
        printf("Unknown skip-hld option %c \n", mgr);
        break;
    }
    i++;
  }
}

/* Parse cmd-line options of bf_switchd */
static void bf_switchd_parse_options(bf_switchd_context_t *ctx,
                                     int argc,
                                     char **argv) {
  char *skip_hld_mgrs_list = NULL;
  while (1) {
    int option_index = 0;
    /* Options without short equivalents */
    enum long_opts {
      OPT_START = 256,
      OPT_INSTALLDIR,
      OPT_CONFFILE,
      OPT_SKIP_P4,
      OPT_SKIP_HLD,
      OPT_SKIP_PORT_ADD,
      OPT_STS_PORT,
      OPT_BACKGROUND,
      OPT_UCLI,
      OPT_BFS_LOCAL,
      OPT_INIT_MODE,
      OPT_NO_PI,
      OPT_P4RT_SERVER,
      OPT_SHELL_NO_WAIT,
      OPT_STATUS_SERVER_LOCALHOST_ONLY,
      OPT_REG_CHANNEL_SERVER_LOCALHOST_ONLY,
      OPT_BF_RT_SERVER_LOCALHOST_ONLY,
      OPT_SERVER_LISTEN_ON_LOCALHOST_ONLY,
    };
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"install-dir", required_argument, 0, OPT_INSTALLDIR},
        {"conf-file", required_argument, 0, OPT_CONFFILE},
        {"skip-p4", no_argument, 0, OPT_SKIP_P4},
        {"skip-hld", required_argument, 0, OPT_SKIP_HLD},
        {"skip-port-add", no_argument, 0, OPT_SKIP_PORT_ADD},
        {"status-port", required_argument, 0, OPT_STS_PORT},
        {"background", no_argument, 0, OPT_BACKGROUND},
        {"ucli", no_argument, 0, OPT_UCLI},
        {"bfs-local-only", no_argument, 0, OPT_BFS_LOCAL},
        {"status-server-local-only",
         no_argument,
         0,
         OPT_STATUS_SERVER_LOCALHOST_ONLY},
        {"reg-channel-server-local-only",
         no_argument,
         0,
         OPT_REG_CHANNEL_SERVER_LOCALHOST_ONLY},
        {"bfrt-grpc-server-local-only",
         no_argument,
         0,
         OPT_BF_RT_SERVER_LOCALHOST_ONLY},
        {"server-listen-local-only",
         no_argument,
         0,
         OPT_SERVER_LISTEN_ON_LOCALHOST_ONLY},
        {"init-mode", required_argument, 0, OPT_INIT_MODE},
        {"p4rt-server", required_argument, 0, OPT_P4RT_SERVER},
        {"shell-no-wait", no_argument, 0, OPT_SHELL_NO_WAIT},
        {0, 0, 0, 0}};
    int c = getopt_long(argc, argv, "h", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case OPT_INSTALLDIR:
	free(ctx->install_dir);
        ctx->install_dir = strdup(optarg);
        break;
      case OPT_CONFFILE:
	free(ctx->conf_file);
        ctx->conf_file = strdup(optarg);
        break;
      case OPT_SKIP_P4:
        ctx->skip_p4 = true;
        break;
      case OPT_SKIP_HLD:
        skip_hld_mgrs_list = strdup(optarg);
	if (skip_hld_mgrs_list) {
            printf("Skip-hld-mgrs list is %s \n", skip_hld_mgrs_list);
            bf_switchd_parse_hld_mgrs_list(ctx, skip_hld_mgrs_list);
            free(skip_hld_mgrs_list);
	}
        break;
      case OPT_SKIP_PORT_ADD:
        ctx->skip_port_add = true;
        break;
      case OPT_STS_PORT:
        ctx->dev_sts_thread = true;
        ctx->dev_sts_port = atoi(optarg);
        break;
      case OPT_BACKGROUND:
        ctx->running_in_background = true;
        break;
      case OPT_UCLI:
        ctx->shell_set_ucli = true;
        break;
      case OPT_BFS_LOCAL:
        ctx->bfshell_local_only = true;
        break;
      case OPT_STATUS_SERVER_LOCALHOST_ONLY:
        ctx->status_server_local_only = true;
        break;
      case OPT_REG_CHANNEL_SERVER_LOCALHOST_ONLY:
        ctx->regular_channel_server_local_only = true;
        break;
      case OPT_BF_RT_SERVER_LOCALHOST_ONLY:
        ctx->bf_rt_server_local_only = true;
        break;
      case OPT_SERVER_LISTEN_ON_LOCALHOST_ONLY:
        ctx->server_listen_local_only = true;
        break;
      case OPT_INIT_MODE:
        if (!strncmp(optarg, "cold", 4)) {
          ctx->init_mode = BF_DEV_INIT_COLD;
        } else if (!strncmp(optarg, "fastreconfig", 4)) {
          ctx->init_mode = BF_DEV_WARM_INIT_FAST_RECFG;
        } else if (!strncmp(optarg, "hitless", 4)) {
          ctx->init_mode = BF_DEV_WARM_INIT_HITLESS;
        } else {
          printf(
              "Unknown init mode, expected one of: \"cold\", \"fastreconfig\", "
              "\"hitless\"\nDefaulting to \"cold\"");
          ctx->init_mode = BF_DEV_INIT_COLD;
        }
        break;
      case OPT_P4RT_SERVER:
	free(ctx->p4rt_server);
        ctx->p4rt_server = strdup(optarg);
        break;
      case OPT_SHELL_NO_WAIT:
        ctx->shell_before_dev_add = true;
        break;
      case 'h':
      case '?':
        printf("bf_switchd \n");
        printf("Usage: bf_switchd --conf-file <file> [OPTIONS]...\n");
        printf("\n");
        printf(" --install-dir=directory that has installed build artifacts\n");
        printf(" --conf-file=configuration file for bf_switchd\n");
        printf(" --tcp-port-base=TCP port base to be used for DMA sim\n");
        printf(" --skip-p4 Skip loading P4 program\n");
        printf(" --skip-hld Skip high level drivers\n");
        printf(
            "   p:pipe_mgr, m:mc_mgr, k:pkt_mgr, r:port_mgr, t:traffic_mgr\n");
        printf(" --skip-port-add Skip adding ports\n");
        printf(" --background Disable interactive features so bf_switchd\n");
        printf("              can run in the background\n");
        printf(" --init-mode Specify cold boot or warm init mode\n");
        printf(
            " cold:Cold boot device, fastreconfig:Apply fast reconfig to "
            "device\n");
        printf(" --p4rt-server=<addr:port> Run the P4Runtime gRPC server\n");
        printf(" --shell-no-wait Start the shell before devices are added\n");
        printf(" -h,--help Display this help message and exit\n");
        exit(c == 'h' ? 0 : 1);
        break;
    }
  }

  /* Sanity check args */
  if ((ctx->install_dir == NULL) || (ctx->conf_file == NULL)) {
    printf("ERROR: --install-dir and --conf-file must be specified\n");
    exit(0);
  }
}

static void bf_switchd_init_sig_set(sigset_t *set) {
  sigemptyset(set);
  sigaddset(set, SIGQUIT);
  sigaddset(set, SIGTERM);
  sigaddset(set, SIGUSR1);
}
static void *bf_switchd_nominated_signal_thread(void *arg) {
  (void)arg;
  sigset_t set;
  siginfo_t info;
  int s, signum;

  bf_switchd_init_sig_set(&set);

  s = pthread_detach(pthread_self());
  if (s != 0) {
    perror("pthread_detach");
    exit(-1);
  }

  for (;;) {
    signum = sigwaitinfo(&set, &info);
    if (signum == -1) {
      if (errno == EINTR) continue;
      perror("sigwait");
      continue;
    }
    switch (signum) {
      case SIGUSR1:
#ifdef COVERAGE_ENABLED
        extern void __gcov_flush(void);
        /* coverage signal handler to allow flush of coverage data*/
        __gcov_flush(); /* dump coverage data on receiving SIGUSR1 */
#endif
        exit(-1);
        break;
      default:
        printf("bf_switchd:received signal %d\n", signum);
        break;
    }
  }
  pthread_exit(NULL);
}

/* Use dedicated signal thread to handle async signal
 * All other threads created by main() will inherit a
 * copy of above signal mask, which blocks those signals.
 *
 * There are synchronous and asynchronous signals.
 *
 * Synchronous signals like SIGSEGV, SIGILL, SIGBUS, SIGFPE, etc
 * are only delivered to the thread that caused it, so if we want to
 * handle those signals we need 1) not mask those signal  and 2) establish a
 * process-wide signal handler with sigaction().
 *
 * Asynchronous signals can be handled by a separate thread, which
 * calls sigwait() and blocks until a signal arrives.
 * To handle those signals we block them in all threads in bf_switchd process,
 * including the thread calling sigwait().
 * This will only work if an asynchronous signal is sent to the whole process,
 * if one thread calls pthread_kill() to send a signal to another thread, or
 * calls raise() which only sends signal to the thread who raises it,
 * that signal will be treated as synchronous.
 */
static void setup_async_signal_handling_thread() {
  sigset_t set;
  bf_switchd_init_sig_set(&set);
  /* add more asynchronous signal if needed
   * exclude: SIGIO  - used in bf_switchd.c
   * exclude: SIGINT - GDB does not catch signal with sigwait
   * */

  /* block the signals in this process by default*/
  int s = pthread_sigmask(SIG_BLOCK, &set, NULL);
  if (s != 0) {
    perror("pthread_sigmask");
    exit(1);
  }
  pthread_t thread;
  s = pthread_create(&thread, NULL, &bf_switchd_nominated_signal_thread, NULL);
  if (s != 0) {
    perror("pthread_create");
    exit(1);
  }
  s = pthread_setname_np(thread, "bf_signal");
  if (s != 0) {
    perror("pthread_setname_np");
    exit(1);
  }
}

/* bf_switchd main */
int main(int argc, char *argv[]) {
  int ret = 0;

  bf_switchd_context_t *switchd_main_ctx = NULL;
  /* Allocate memory to hold switchd configuration and state */
  if ((switchd_main_ctx = malloc(sizeof(bf_switchd_context_t))) == NULL) {
    printf("ERROR: Failed to allocate memory for switchd context\n");
    return -1;
  }
  memset(switchd_main_ctx, 0, sizeof(bf_switchd_context_t));

  setup_async_signal_handling_thread();

  /* Parse bf_switchd arguments */
  bf_switchd_parse_options(switchd_main_ctx, argc, argv);

  ret = bf_switchd_lib_init(switchd_main_ctx);
  pthread_join(switchd_main_ctx->tmr_t_id, NULL);
  pthread_join(switchd_main_ctx->accton_diag_t_id, NULL);

  if (switchd_main_ctx) {
    free(switchd_main_ctx->p4rt_server);
    free(switchd_main_ctx->install_dir);
    free(switchd_main_ctx->conf_file);
    free(switchd_main_ctx);
  }

  return ret;
}
