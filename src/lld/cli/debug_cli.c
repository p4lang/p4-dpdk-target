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
#include <Python.h>
#include <stdio.h>
#include "target-utils/clish/shell.h"
#include <dvm/bf_drv_intf.h>
#include <target-sys/bf_sal/bf_sys_sem.h>
#include <lld/python_shell_mutex.h>
/*
 * Start BF Runtime CLI. This program heavily borrows from the python C
 * interface example program.
 */
static int debug_start_cli(int in_fd,
                           int out_fd,
                           const char *install_dir,
                           const char *udf) {
  PyObject *pName = NULL, *pModule = NULL, *pFunc = NULL;
  PyObject *pArgs = NULL, *pValue = NULL;

  Py_Initialize();

  /* Load the debugcli python program. Py_Initialize loads its libraries from
  the install dir we installed Python into. */
  pName = PyUnicode_DecodeFSDefault("debugcli");
  if (pName) {
      pModule = PyImport_Import(pName);
      Py_DECREF(pName);
  }

  if (pModule != NULL) {
    // Create a call to the start_debug_py function in bfrtcli.py
    pFunc = PyObject_GetAttrString(pModule, "start_debug_py");
    /* pFunc is a new reference */

    if (pFunc && PyCallable_Check(pFunc)) {
      // Use a tuple as our argument list
      if (udf) {
        pArgs = PyTuple_New(4);
	if (!pArgs) {
	    printf("Allocation of pArgs failed.\n");
	    Py_Finalize();
	    return 1;
	}
      } else {
        // pArgs = PyTuple_New(3);
        // something is not right return
        printf("debug python cli requires a py_file.\n");
        Py_Finalize();
        return 1;
      }
      // Create python objects from c types.
      // Place references to them in the argument tuple.
      pValue = PyLong_FromLong(in_fd);
      PyTuple_SetItem(pArgs, 0, pValue);
      pValue = PyLong_FromLong(out_fd);
      PyTuple_SetItem(pArgs, 1, pValue);
      /*
       * Convert from the filepath c string to a python string using the
       * filesystem's default encoding
       */
      pValue = PyUnicode_DecodeFSDefault(install_dir);
      PyTuple_SetItem(pArgs, 2, pValue);
      pValue = PyUnicode_DecodeFSDefault(udf);
      PyTuple_SetItem(pArgs, 3, pValue);

      // Actually make the function call.
      // This is where we block until the CLI exits
      pValue = PyObject_CallObject(pFunc, pArgs);
      Py_DECREF(pArgs);

      // Handle exit codes and decrement references to free our python objects
      if (pValue != NULL) {
        long ret = PyLong_AsLong(pValue);
        if (ret != 0) {
          printf("debug cli exited with error: %ld\n", ret);
        }
        Py_DECREF(pValue);
      } else {
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        PyErr_Print();
        fprintf(stderr, "debug cli python call failed\n");
        Py_Finalize();
        return 1;
      }
    } else {
      if (PyErr_Occurred()) PyErr_Print();
      fprintf(stderr, "Cannot find start_debug function.\n");
    }
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
  } else {
    PyErr_Print();
    fprintf(stderr, "Failed to load debugcli python library\n");
    Py_Finalize();
    return 1;
  }
  // This will free all remaining memory allocated by python & the CLI.
  Py_Finalize();
  return 0;
}

// A klish plugin allowing BF Debug Python CLI to be started from bfshell.
CLISH_PLUGIN_SYM(debug_cli_cmd) {
  (void)script;
  (void)out;
  bf_status_t sts;
  clish_shell_t *bfshell = clish_context__get_shell(clish_context);
  /* init python shell mutex, per process/shell */
  bool success = TRY_PYTHON_SHL_LOCK();  // share with any other python shell
  if (!success) {
    bfshell_printf(clish_context,
                   "Only one Python shell instance allowed at a time. bfrt "
                   "python and debug python share the python shell "
                   "resource.\n");
    return 0;
  }

  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);
  const clish_parg_t *parg = clish_pargv_find_arg(pargv, "py_file");
  const char *udf = NULL;
  if (parg) {
    udf = clish_parg__get_value(parg);
  } else {
    bfshell_printf(clish_context, "Only support running through py_file.\n");
    goto end;
  }

  tinyrl_t *bftinyrl = clish_shell__get_tinyrl(bfshell);

  sts = debug_start_cli(fileno(tinyrl__get_istream(bftinyrl)),
                        fileno(tinyrl__get_ostream(bftinyrl)),
                        clish_context__get_install_dir(clish_context),
                        udf);
  if (sts != BF_SUCCESS) {
    bfshell_printf(clish_context,
                   "%s:%d could not initialize bf_rt for the cli. err: %d\n",
                   __func__,
                   __LINE__,
                   sts);
  }
end:
  RELEASE_PYTHON_SHL_LOCK();
  return 0;
}

CLISH_PLUGIN_INIT(debug) {
  (void)clish_shell;
  clish_plugin_add_sym(plugin, debug_cli_cmd, "debug_cli_cmd");
  return 0;
}

void debug_cli_err_str(bf_status_t sts, const char **err_str) {
  *err_str = bf_err_str(sts);
}
