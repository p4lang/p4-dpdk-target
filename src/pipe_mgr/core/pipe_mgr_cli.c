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
#include "target_utils/clish/shell.h"
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
