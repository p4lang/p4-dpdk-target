/*
 * SPDX-FileCopyrightText: 2021 Intel Corporation
 * Copyright (C) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "target-utils/clish/shell.h"
#include <dvm/bf_drv_intf.h>
#include <stdio.h>

CLISH_PLUGIN_SYM(ucli_cmd) {
  (void)script;
  (void)out;
  clish_shell_t *shell = clish_context__get_shell(clish_context);
  tinyrl_t *tinyrl = clish_shell__get_tinyrl(shell);
  bf_drv_ucli_run(tinyrl__get_istream(tinyrl), tinyrl__get_ostream(tinyrl));
  return 0;
}

CLISH_PLUGIN_SYM(version_cmd) {
  (void)script;
  (void)out;

  const char *install_dir = clish_context__get_install_dir(clish_context);
  char *version_path = calloc(1024, sizeof(char));
  if (version_path == NULL) {
    printf("Cannot allocate memory for version path buffer\n");
    return 0;
  }
  snprintf(version_path, 1023, "%s%s", install_dir, "share/VERSION");

  FILE *fptr = fopen(version_path, "r");
  if (fptr == NULL) {
    printf("Cannot open version file: %s\n", version_path);
    free(version_path);
    return 0;
  }

  char c = fgetc(fptr);
  while (!feof(fptr)) {
    bfshell_printf(clish_context, "%c", c);
    c = fgetc(fptr);
  }
  fclose(fptr);
  free(version_path);

  return 0;
}

CLISH_PLUGIN_SYM(pipemgr_show_tcams) {
  (void)script;
  (void)out;
  char *device;
  device = (char *)clish_shell_expand_var_ex(
      "device_id", clish_context, SHELL_EXPAND_VIEW);
  bfshell_printf(clish_context, "Tcams for device %s\n", device);
  bfshell_string_free(device);
  return 0;
}

CLISH_PLUGIN_INIT(pipemgr) {
  (void)clish_shell;
  clish_plugin_add_sym(plugin, pipemgr_show_tcams, "pipemgr_show_tcams");
  clish_plugin_add_sym(plugin, ucli_cmd, "ucli_cmd");
  clish_plugin_add_sym(plugin, version_cmd, "version_cmd");
  return 0;
}
