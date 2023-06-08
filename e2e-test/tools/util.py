import logging
import os
import shutil
from pathlib import Path
from subprocess import Popen, run


def assert_exists(path):
    path = Path(path)
    assert path.exists(), f"{path} doesn't exist"


def bf_prepare(test_dir, bin_name):
    sde_env = get_sde_env()
    sde_install = sde_env["SDE_INSTALL"]
    bin_path = Path(f"{sde_install}/bin/{bin_name}")
    assert bin_path.exists(), f"Required binary {bin_path} doesn't exist"
    log_dir = test_dir / f"log/{bin_name}/"
    log_dir.mkdir(parents=True, exist_ok=True)
    return bin_path, log_dir, sde_env


def bf_run(cmd, log_dir, sde_env, in_background, with_sudo=False):
    if with_sudo:
        # Passing environment variables this way is necessary. See
        # https://github.com/Yi-Tseng/p4-dpdk-target-notes#start-the-switch
        sudo = [
            "sudo",
            "-E",
            f"PATH={sde_env['PATH']}",
            f"LD_LIBRARY_PATH={sde_env['LD_LIBRARY_PATH']}",
        ]
        cmd = sudo + cmd
    log_cmd(cmd)
    stdout = None
    if in_background:
        stdout = log_dir / "stdout.log"
        stderr = log_dir / "stderr.log"
        # Set bufsize to 0 because we need to check stdout in real time later
        p = Popen(
            cmd,
            bufsize=0,
            stdout=stdout.open("w"),
            stderr=stderr.open("w"),
            cwd=log_dir,
            env=sde_env,
        )
    else:
        p = run(cmd, cwd=log_dir, env=sde_env, check=True)
    logging.info(f"PID: {p.pid}")
    return p, stdout


def get_sde_env():
    sde_env = {}
    sde_env.update(os.environ)
    for env_var in ["SDE", "SDE_INSTALL"]:
        assert env_var in sde_env, f"Environment variable {env_var} not set"
        logging.info(f"Using {env_var}: {sde_env[env_var]}")
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
        cmd = [arg.as_posix() if isinstance(arg, Path) else arg for arg in cmd]
        cmd = " ".join(cmd)
    logging.info(f"Run command:\n{cmd}")


def remove_file_or_dir(path):
    path = Path(path)

    if not path.exists():
        return

    if path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)
