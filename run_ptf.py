import time
import yaml
import json
import sys
import argparse
import os
import signal
import subprocess
import datetime

# set to avoid too much traceback to be printed; sys.tracebacklimit = 0
dt = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
DEFAULT_TIMEOUT="180"

def start_bf_switchd(sde_install, p4_program_name):
    bf_switchd_app = os.path.join(sde_install,"bin", "bf_switchd")
    conf_file = os.path.join(sde_install, "sample_app", p4_program_name, ""+p4_program_name+".conf")
    start_bf_switchd_cmd = ""+bf_switchd_app+" --install-dir "+sde_install+" --conf-file "+conf_file+" --init-mode=cold --status-port 7777 --background &"
    print (start_bf_switchd_cmd)
    bfswitch_start = Command(start_bf_switchd_cmd)
    bfswitch_start.run_bashcmd()

def run_stage(sde, sde_install, p4_program_name, step_name):
    RUN_P4_TEST = os.path.join(sde_install, "bin", "run_p4_tests.sh")
    script_path = os.path.join(sde, "driver", "third-party" , "ptf_grpc", "ptf-tests", "switchd")
    stage_cmd = ""+RUN_P4_TEST+" --default-timeout "+DEFAULT_TIMEOUT+" -p "+p4_program_name+" -t "+script_path+" --no-veth -s "+step_name+""
    print (stage_cmd)
    run_stage = Command(stage_cmd)
    run_stage.run_bashcmd()

def run_verify_stage(sde, sde_install, p4_program_name, step_name):
    RUN_P4_TEST = os.path.join(sde_install, "bin", "run_p4_tests.sh")
    script_path = os.path.join(sde, "driver", "third-party" , "ptf_grpc", "ptf-tests", "switchd")
    port_info = os.path.join(script_path, "port_info.json")
    stage_cmd = ""+RUN_P4_TEST+" --default-timeout "+DEFAULT_TIMEOUT+" -p "+p4_program_name+" -t "+script_path+" --no-veth -f "+port_info+" -s "+step_name+""
    run_stage = Command(stage_cmd)
    run_stage.run_bashcmd()

def print_error_msg(msg, status=1):
    print(msg)
    sys.exit(status)

class Command(object):
    def __init__(self,cmd):
        self.cmd = cmd
        self.rc = 0;

    def run_popen(self, shell=True, env=os.environ):
        try:
            sys.stdout.flush()
            print(f'\tRunning cmd: {self.cmd}')
            p1=subprocess.Popen(self.cmd, shell=shell,stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=env)
            (self.out, self.err) = p1.communicate()
            self.rc = p1.returncode
            return self
        except Exception as e:
            print_error_msg(str(e))

    def run_bashcmd(self, print_cmd=False, shell=True, exitonfail=False, dry_run=False, check_output=False):
        if print_cmd:
            print(f'Running cmd: {self.cmd}')
        if not dry_run:
            try:
                if check_output:
                    self.rc = subprocess.check_call(self.cmd, shell=shell, stdout=subprocess.DEVNULL)
                else:
                    self.rc = subprocess.call(self.cmd, shell=shell, stderr=subprocess.STDOUT)
            except subprocess.CalledProcessError as exc:
                print (f'Error code while running cmd: {self.cmd}')
                self.rc = exc.returncode
                print (f'Error code: {exc.returncode}')
                print (f'Exception is: {exc}')
                if exitonfail:
                    sys.exit(1)
            finally:
                return self.rc

def dir_path(string):
    if os.path.isdir(string):
        return string
    else:
        print ("Invalid Path: %s " %(string))
        raise NotADirectoryError(string)

def file_path(string):
    if os.path.isfile(string):
        return string;
    else:
        raise FileNotFoundError(string)

def inputs():
    parser = argparse.ArgumentParser()
    parser.add_argument('--sde', type = dir_path, required=False, dest = "sde", default= os.getcwd(),
                        help='provide the full direcorty path containing the files')
    parser.add_argument('--sde_install', type = dir_path, required=False, dest = "sde_install", default=os.path.join(os.getcwd(),"install") ,
                        help='provide the full direcorty path containing the files')
    parser.add_argument('--ptf_config', type = file_path, required=False, dest = "ptf_config", default=os.path.join(os.getcwd(), "ptf-config.yml") ,
                        help='provide the full direcorty path containing the files')
    args = parser.parse_args()
    return (args)

def set_env(sde, sde_install):
    try:
        os.environ["SDE"]
        print ("Environment SDE is already set with %s ", (sde))
    except:
        print ("setting environment variable SDE %s ", (sde))
        os.environ["SDE"] = sde
    try:
        os.environ["SDE_INSTALL"]
        print ("Environment SDE_INSTALL is already set with %s ", (sde_install))
    except:
        print ("setting environment variable SDE_INSTALL %s ", (sde_install))
        os.environ["SDE_INSTALL"] = sde_install

    if (os.environ["SDE_INSTALL"]) and (os.environ["SDE"]):
        os.environ["LD_LIBRARY_PATH"] = ""+sde_install+"/lib/:"+sde_install+"/lib/x86_64-linux-gnu:"+sde_install+"/lib64"
        os.environ["PKG_CONFIG_PATH"] = ""+sde_install+"/lib/x86_64-linux-gnu/pkgconfig:"+sde_install+"/lib64/pkgconfig"

    print (os.environ)

def kill_linux_process(process_name):
    try:
        for process in os.popen("ps ax | grep " + process_name + " | grep -v grep"):
            os.kill(int(process.split()[0]), signal.SIGKILL)
        print("Process %s Successfully terminated " %(process_name))
    except:
        print("Something went wrong while trying to kill the %s " %(process_name))

def create_report_templates(test_doc):
    json_data = {}
    for key,value in test_doc.items():
        if key == "stages":
            stages = []
            for each_stage in value:
                stage = {}
                stage[each_stage] = "None"
                stages.append(stage)
            json_data[key] = stages
        else:
            json_data[key] = value
        file_name = os.path.join("json_reports", test_doc['name'])
        print (file_name)
        with open (file_name, 'w') as jout:
            json.dump(json_data, jout)

def parse_ptf_config(ptf_config_filepath):
    with open("ptf-config.yml", "r") as stream:
        try:
            docs = yaml.load_all(stream,Loader=yaml.FullLoader)
        except yaml.YAMLError as err:
            print(err)

        for doc in docs:
            create_report_templates(doc)

def run_reports(args, test_data, file_name):
    for item in test_data['stages']:
        if "verify" in list(item.keys())[0]:
            run_verify_stage(args.sde, args.sde_install, os.path.basename(file_name), list(item.keys())[0])
        else:
            run_stage(args.sde, args.sde_install, os.path.basename(file_name), list(item.keys())[0])

def create_artifacts():
    cmd = Command("./install/bin/gen_p4_artifacts.sh")
    cmd.run_bashcmd()

def mark_active(report):
    with open("running", "w") as f:
        f.write(report)
        
def merge_jsonfiles():
    results = {}
    for report in os.listdir('json_reports'):
        rep_path = os.path.join('json_reports',report)
        with open(rep_path, 'r') as infile:
            data = (json.load(infile))
            results[data['name']] = data['stages']
        
    with open('testresults.json', 'w') as outfile:
        json.dump(results, outfile, indent=1)

def delete_jsonfiles():
    for report in os.listdir('json_reports'):
        rep_path = os.path.join('json_reports',report)
        try:
            os.remove(rep_path)
        except Exception as e:
            print ("failed to delete file %s ", (rep_path))
            print (str(e))
     
def main():
    args = inputs()
    if args.sde and args.sde_install:
        set_env(args.sde, args.sde_install)
    if os.path.isdir("json_reports"):
        print ("folder exists")
    else:
        os.mkdir("json_reports")
    print (os.listdir())
    if args.ptf_config:
        parse_ptf_config(args.ptf_config)

    for report in os.listdir('json_reports'):
        file_name = os.path.join("json_reports", report)
        print (file_name)
        kill_linux_process("bf_switchd")
        time.sleep(5)
        mark_active(report)
        start_bf_switchd(args.sde_install, report)
        with open(file_name) as json_file:
            data = json.load(json_file)
            run_reports(args, data, file_name)
        kill_linux_process("bf_switchd")
        time.sleep(5)
        
    merge_jsonfiles()
    delete_jsonfiles()

main()
