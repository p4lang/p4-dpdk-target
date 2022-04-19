import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--p4-name", required=True,
                     help="Name of the P4 program")
parser.add_argument("--bf-rt-dir", required=True,
                     help="path of bf-rt.json")
parser.add_argument("--context-dir", required=True,
                     help="path of context.json")
parser.add_argument("--spec-dir", required=True,
                     help="path of spec file")
args = parser.parse_args()

bfrt = open(args.bf_rt_dir)
bfrt_json = json.load(bfrt)

#This func sets the path to store conf file
def set_conf():
    conf_path = args.bf_rt_dir.split("/")
    conf = ""
    i = 0
    while conf_path[i] != 'bf-rt.json':
        conf += conf_path[i] + "/"
        i += 1
    print("Conf File stored in -->",conf+args.p4_name+".conf")
    return conf+args.p4_name+".conf"

#This func sets the pipe_name in the conf file
def get_pipe_name():
    if 'tables' in bfrt_json:
        return bfrt_json['tables'][-1].get('name').split(".")[0]
    else:
        print("Error: Not able to find the tables")
        return NULL

#This funct set the context.json path for the conf file
def get_context():
    context_path = ""
    path = args.context_dir.split("/")
    path = path[path.index('install')+1:]
    context_path = path[0]
    for i in range(1 , len(path)):
        context_path += "/" + path[i]
    return context_path

#This func sets the spec file path in the conf file
def get_spec():
    spec_path = ""
    path = args.spec_dir.split("/")
    path = path[path.index('install')+1:]
    spec_path =  path[0]
    for i in range(1 , len(path)):
        spec_path += "/" + path[i]
    return spec_path

#Get the P4 Name to set in the conf file
def get_p4name():
    return args.p4_name

#Get the bf-rt.json path for the conf file
def bfrt_path():
    bfrt_path = ""
    path = args.bf_rt_dir.split("/")
    path = path[path.index('install')+1:]
    bfrt_path = path[0]
    for i in range(1 , len(path)):
        bfrt_path += "/" + path[i]
    return bfrt_path

#this func sets the path in the conf file
def get_path():
    return "sample_app/"+args.p4_name

#this is the schema for the conf file, Please traverse it from below
chip_list = []
list = {
        "id": "asic-0",
        "chip_family": "dpdk",
        "instance": 0
        }
chip_list.append(list)

p4_devices = []

mempool_data = [{
    "name": "MEMPOOL0",
    "buffer_size": 2304,
    "pool_size": 1024,
    "cache_size": 256,
    "numa_node": 0
    }]

scope = [
        0,
        1,
        2,
        3
        ]

pipeline = [{
    "p4_pipeline_name": get_pipe_name(),
    "core_id": 1,
    "numa_node": 0,
    "context": get_context(),
    "config": get_spec(),
    "pipe_scope" : scope,
    "path" : get_path()
    }]

p4_program = [{
    "program-name": get_p4name(),
    "bfrt-config": bfrt_path(),
    "p4_pipelines": pipeline,
    }]

devices = {
        "device-id": 0,
        "eal-args": "dummy -n 4 -c 3",
        "mempools": mempool_data,
        "p4_programs": p4_program
        }
p4_devices.append(devices)

data = {
        "chip_list" : chip_list,
        "instance" : 0,
        "p4_devices" : p4_devices,
        }

#after populating the schema write it in the conf file
with open(set_conf(), 'w') as f:
    json.dump(data, f, indent=4)
