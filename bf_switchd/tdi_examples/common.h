#include <getopt.h>
#include <unistd.h>

#include <bf_switchd/bf_switchd.h>
#include <target-utils/clish/thread.h>
#include <dvm/bf_drv_intf.h>

/* A small enum capturing the various options for interactive CLI sessions used
 * by the program. */
enum cli_modes { no_interactive_cli, interactive_ucli, interactive_bfshell };

/* A couple global variables used to pass state between the init function
 * (parse_opts_and_switchd_init) and the final cleanup function
 * (run_cli_or_cleanup) so the caller does not need to bother with additional in
 * and out arguments to the calls. */
static enum cli_modes requested_cli_mode = no_interactive_cli;
static bf_switchd_context_t *switchd_ctx = NULL;

static void parse_options(int argc, char **argv) {
  int option_index = 0;
  enum opts { OPT_INSTALLDIR = 1, OPT_CONFFILE, OPT_BFSHELL, OPT_UCLI };
  static struct option options[] = {
      {"help", no_argument, 0, 'h'},
      {"install-dir", required_argument, 0, OPT_INSTALLDIR},
      {"conf-file", required_argument, 0, OPT_CONFFILE},
      {"bfshell", no_argument, 0, OPT_BFSHELL},
      {"ucli", no_argument, 0, OPT_UCLI},
      {NULL, 0, 0, 0}};

  while (1) {
    int c = getopt_long(argc, argv, "h", options, &option_index);

    if (c == -1) {
      break;
    }
    switch (c) {
      case OPT_INSTALLDIR:
        switchd_ctx->install_dir = optarg;
        printf("Install Dir: %s\n", switchd_ctx->install_dir);
        break;
      case OPT_CONFFILE:
        switchd_ctx->conf_file = optarg;
        printf("Conf-file  : %s\n", switchd_ctx->conf_file);
        break;
      case OPT_BFSHELL:
        printf("Interactive bf-shell requested\n");
        requested_cli_mode = interactive_bfshell;
        break;
      case OPT_UCLI:
        printf("Interactive uCLI requested\n");
        requested_cli_mode = interactive_ucli;
        break;
      case 'h':
      case '?':
        printf("%s\n", argv[0]);
        printf(
            "Usage : %s --install-dir=<SDE install path> --conf-file=<path to "
            "conf file> [--ucli|--bfshell]\n",
            argv[0]);
        exit(c == 'h' ? 0 : 1);
        break;
      default:
        printf("Invalid option\n");
        exit(0);
        break;
    }
  }
  if (switchd_ctx->install_dir == NULL) {
    printf("ERROR : --install-dir must be specified\n");
    exit(0);
  }

  if (switchd_ctx->conf_file == NULL) {
    printf("ERROR : --conf-file must be specified\n");
    exit(0);
  }
}

static void parse_opts_and_switchd_init(int argc, char **argv) {
  /* Check if root privileges exist or not, exit if not. */
  if (geteuid() != 0) {
    printf("Need to run as root user! Exiting !\n");
    exit(1);
  }

  /* Allocate memory for the libbf_switchd context. */
  switchd_ctx = (bf_switchd_context_t *)calloc(1, sizeof(bf_switchd_context_t));
  if (!switchd_ctx) {
    printf("Cannot Allocate switchd context\n");
    exit(1);
  }

  /* Always set "background" because we do not want bf_switchd_lib_init to start
   * a CLI session.  That can be done afterward by the caller if requested
   * through command line options. */
  switchd_ctx->running_in_background = true;

  /* Always set "skip port add" so that ports are not automatically created when
   * running on either model or HW. */
  switchd_ctx->skip_port_add = true;

  /* Parse command line options. */
  parse_options(argc, argv);

  /* Initialize libbf_switchd. */
  bf_status_t status = bf_switchd_lib_init(switchd_ctx);
  if (status != BF_SUCCESS) {
    printf("Failed to initialize libbf_switchd (%s)\n", bf_err_str(status));
    free(switchd_ctx);
    exit(1);
  }
}

static void run_cli_or_cleanup() {
  /* Start a CLI shell if one was requested. */
  if (requested_cli_mode == interactive_ucli) {
    bf_drv_shell_start();
  } else if (requested_cli_mode == interactive_bfshell) {
    cli_run_bfshell();
  }

  /* If we started a CLI shell, wait to exit. */
  if (requested_cli_mode != no_interactive_cli) {
    pthread_join(switchd_ctx->tmr_t_id, NULL);
#if 0
    //pthread_join(switchd_ctx->dma_t_id, NULL);
    //pthread_join(switchd_ctx->int_t_id, NULL);
    //pthread_join(switchd_ctx->pkt_t_id, NULL);
    //pthread_join(switchd_ctx->port_fsm_t_id, NULL);
    //pthread_join(switchd_ctx->drusim_t_id, NULL);
    for (size_t i = 0;
         i < sizeof switchd_ctx->agent_t_id / sizeof switchd_ctx->agent_t_id[0];
         ++i) {
      if (switchd_ctx->agent_t_id[i] != 0) {
        pthread_join(switchd_ctx->agent_t_id[i], NULL);
      }
    }
#endif
  }

  free(switchd_ctx);
}
