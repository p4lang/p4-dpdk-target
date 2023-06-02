import logging
import os
import shutil
from pathlib import Path


def get_sde_env():
    sde_env = {}
    sde_env.update(os.environ)
    for env_var in ["SDE", "SDE_INSTALL"]:
        assert env_var in sde_env, f"Environment variable {env_var} not set"
        print(f"Using {env_var}: {sde_env[env_var]}")
    sde_install = sde_env["SDE_INSTALL"]
    sde_env["LD_LIBRARY_PATH"] = f"{sde_install}/lib"
    sde_env["LD_LIBRARY_PATH"] += f":{sde_install}/lib64"
    sde_env["LD_LIBRARY_PATH"] += f":{sde_install}/lib/x86_64-linux-gnu"
    sde_env["PYTHONPATH"] = f"{sde_install}/lib/python3.10"
    sde_env["PYTHONPATH"] += f":{sde_install}/lib/python3.10/lib-dynload"
    sde_env["PYTHONPATH"] += f":{sde_install}/lib/python3.10/site-packages"
    sde_env["PYTHONHOME"] = f"{sde_install}/lib/python3.10"
    return sde_env


def log_cmd(cmd):
    if isinstance(cmd, list):
        cmd = " ".join(cmd)
    logging.info(f"Run command: {cmd}")


def remove_file_or_dir(path):
    path = Path(path)

    if not path.exists():
        return

    if path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)
