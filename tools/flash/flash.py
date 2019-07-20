#
# @file from https://github.com/Neutree/c_cpp_project_framework
# @author neucrack
#

from __future__ import print_function
import argparse
import os, sys, time, re, shutil
import subprocess
from multiprocessing import cpu_count
import json
import serial.tools.miniterm


parser = argparse.ArgumentParser(add_help=False, prog="flash.py")

############################### Add option here #############################
boards_choices = ["dan", "bit", "bit_mic", "goE", "goD", "maixduino", "kd233", "auto"]
parser.add_argument("-p", "--port", help="[flash] device port", default="")
parser.add_argument("-b", "--baudrate", type=int, help="[flash] baudrate", default=115200)
parser.add_argument("-t", "--terminal", help="[flash] start a terminal after finish (Python miniterm)", default=False, action="store_true")
parser.add_argument("-n", "--noansi", help="[flash] do not use ANSI colors, recommended in Windows CMD", default=False, action="store_true")
parser.add_argument("-s", "--sram", help="[flash] download firmware to SRAM and boot", default=False, action="store_true")
parser.add_argument("-B", "--Board",required=False, type=str, default="auto", help="[flash] select dev board, e.g. -B bit", choices=boards_choices)
parser.add_argument("-S", "--Slow",required=False, help="[flash] slow download mode", default=False, action="store_true")
dict_arg = {"port":"", 
            "baudrate": 115200,
            "terminal": False,
            "noansi": False,
            "sram": False,
            "Board": "auto",
            "Slow": False
            }
dict_arg_not_save = ["sram", "terminal", "Slow"]

def kflash_py_printCallback(*args, **kwargs):
    end = kwargs.pop("end", "\n")
    msg = ""
    for i in args:
        msg += str(i)
    msg.replace("\n", " ")
    print(msg, end=end)

def kflash_progress(fileTypeStr, current, total, speedStr):
    # print("{}/{}".format(current, total))
    pass
#############################################################################

# use project_args created by SDK_PATH/tools/cmake/project.py

# args = parser.parse_args()
if __name__ == '__main__':
    firmware = ""
    try:
        flash_conf_path = project_path+"/.flash.conf.json"
        if project_args.cmd == "clean_conf":
            if os.path.exists(flash_conf_path):
                os.remove(flash_conf_path)
            exit(0)
        if project_args.cmd != "flash":
            print("call flash.py error")
            exit(1)
    except Exception:
        print("-- call flash.py directly!")
        parser.add_argument("firmware", help="firmware file name")
        project_parser = parser
        project_args = project_parser.parse_args()
        project_path = ""
        if not os.path.exists(project_args.firmware):
            print("firmware not found:{}".format(project_args.firmware))
            exit(1)
        firmware = project_args.firmware
        sdk_path = ""

    config_old = {}
    # load flash config from file
    try:
        with open(flash_conf_path, "r") as f:
            config_old = json.load(f)
    except Exception as e:
        pass
    # update flash config from args
    for key in dict_arg.keys():
        dict_arg[key] = getattr(project_args, key)
    # check if config update, if do, use new and update config file
    config = {}
    for key in config_old:
        config[key] = config_old[key]
    for key in dict_arg.keys():
        if dict_arg[key] != project_parser.get_default(key): # arg valid, update config
            config[key] = dict_arg[key]
        else:
            if not key in config:
                config[key] = dict_arg[key]
    if config != config_old:
        print("-- flash config changed, update at {}".format(flash_conf_path))
        with open(flash_conf_path, "w+") as f:
            json.dump(config, f, indent=4)
    # mask options that not read from file
    for key in config:
        if key in dict_arg_not_save:
            config[key] = dict_arg[key]
    print("-- flash start")
    ############## Add flash command here ################
    if project_path != "":
        firmware = project_path+"/build/"+project_name+".bin"
    if not os.path.exists(firmware):
        print("[ERROR] Firmware not found:{}".format(firmware))
        exit(1)
    if config["port"] == "":
        print("[ERROR] Invalid port:{}, set by -p or --port, e.g. -p /dev/ttyUSB0".format(config["port"]))
        exit(1)
    print("=============== flash config =============")
    print("-- flash port    :{}".format(         config["port"]       ))
    print("-- flash baudrate:{}".format(         config["baudrate"]   ))
    print("-- flash board:{}".format(            config["Board"]      ))
    print("-- flash open terminal:{}".format(    config["terminal"]   ))
    print("-- flash download to sram:{}".format( config["sram"]       ))
    print("-- flash noansi:{}".format(           config["noansi"]     ))
    print("-- flash slow mode:{}".format(        config["Slow"]       ))
    print("-- flash firmware:{}".format( firmware ))
    print("")
    print("-- kflash start")
    # call kflash to burn firmware
    from kflash_py.kflash import KFlash 

    kflash = KFlash(print_callback=kflash_py_printCallback)
    flash_success = True
    err_msg = ""
    try:
        if config["Board"]=="auto":
            kflash.process(terminal=False, dev=config["port"], baudrate=config["baudrate"], \
                sram = config["sram"], file=firmware, callback=kflash_progress, noansi= config["noansi"], \
                terminal_auto_size=True, slow_mode = config["Slow"])
        else:
            kflash.process(terminal=False, dev=config["port"], baudrate=config["baudrate"], board=config["Board"], \
                sram = config["sram"], file=firmware, callback=kflash_progress, noansi= config["noansi"], \
                terminal_auto_size=True, slow_mode = config["Slow"])
    except Exception as e:
        flash_success = False
        err_msg = str(e)
    if not flash_success:
        print("[ERROR] flash firmware fail:")
        print("     "+err_msg)
        exit(1)
    ######################################################    
    print("== flash end ==")    

    ######################################################
    # open serial tool
    if config["terminal"]:
        reset = True
        if config["sram"]:
            reset = False
        sys.argv=[project_path+"/tools/flash/flash.py"]
        serial.tools.miniterm.main(default_port=config["port"], default_baudrate=115200, default_dtr=reset, default_rts=reset)
        

    ######################################################
    

