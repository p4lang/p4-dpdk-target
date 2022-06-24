import os
import json
        
class REPORTING():
    def print_result(result):
        print ("PTF test result: %s ", (result))

    def get_json():
        try:
            install_dir = os.environ.get("SDE_INSTALL")
            sde_dir = os.path.dirname(install_dir)
            run_file = os.path.join(sde_dir, "running")
            with open (run_file, 'r') as rf:
                p4_program = rf.read()
            return os.path.join(sde_dir, "json_reports", p4_program)
        except Exception as e:
            print (str(e))
            print ("!!!! Could not find any json file, json report data will not be updated  !!!")
            print ("Could not find the running file")
            return None

    def update_data(json_data, step_name, status):
        updated_stages = []
        for item in (json_data['stages']):
            if list(item.keys())[0] == step_name:
                item[step_name] = status
                updated_stages.append(item)
            else:
                updated_stages.append(item)
        json_data['stages'] = updated_stages
        return json_data

    def update_results(json_file, step_name, result):
        REPORTING.print_result(result)
        try:
            with open(json_file, "r") as jsonFile:
                data = json.load(jsonFile)
            updated_data = REPORTING.update_data(data, step_name, result)
        except Exception as e:
            print (str(e))
            print ("--!! Failed to update results in json !!--")
            print ("no json file found: ", json_file)
        try:
            with open(json_file, "w") as jsonFile:
                json.dump(updated_data, jsonFile)
        except Exception as e:
            print (str(e))
