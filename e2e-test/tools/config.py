from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent

# Test suite path conventions

SUITE_MAIN_P4 = "main.p4"
SUITE_CMDS_BFSHELL = "cmds_bfshell.py"
SUITE_CMDS_SHELL = "cmds_shell.sh"
SUITE_CONF_BF_SWITCHD = "conf_bf_switchd.json"
SUITE_TEST = "test.py"

SUITE_P4C_GEN = "p4c_gen"
SUITE_P4INFO = SUITE_P4C_GEN + "/p4info.txt"
SUITE_BFRT = SUITE_P4C_GEN + "/bfrt.json"
SUITE_TDI = SUITE_P4C_GEN + "/tdi.json"
SUITE_CONTEXT = SUITE_P4C_GEN + "/context.json"
SUITE_SPEC = SUITE_P4C_GEN + "/config.spec"

SUITE_LOG = "log"

SUITE_PATH_GENERATED = [SUITE_CONF_BF_SWITCHD, SUITE_P4C_GEN, SUITE_LOG]
