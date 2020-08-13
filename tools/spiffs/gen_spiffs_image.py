
from __future__ import print_function
import argparse
import os
import re

in_dir  = "fs"
out_dir = "fs_image"
out_name = "maixpy_spiffs.img"

parser = argparse.ArgumentParser(description='generate spiffs image', prog="gen_spiffs_image.py")
parser.add_argument("config_file", help = "project config file path, MUST the same with firmware's", default="")

args = parser.parse_args()

if args.config_file == "" or not os.path.exists(args.config_file):
    print("file {} not found".format(args.config_file))
    exit(1)

if not os.path.exists(in_dir):
    print("file system dir not found:"+in_dir)
    print("please put files in "+in_dir+" folder")
    os.mkdir(in_dir)
    exit(1)

if len(os.listdir(in_dir)) == 0:
    print("[WARNING] No file in fs dir, so the image will be empty")
 
# check submodule
if len(os.listdir("mkspiffs")) == 0:
    ret = os.system("git submodule update --init mkspiffs")
    if ret != 0:
        print("[ERROR] update submodule mkspiffs fail")
        exit(1)

if len(os.listdir("mkspiffs/spiffs")) == 0:
    os.chdir("mkspiffs")
    ret = os.system("git submodule update --init spiffs")
    if ret != 0:
        print("[ERROR] update submodule mkspiffs/spiffs fail")
        exit(1)
    os.chdir("..")

f = open(args.config_file)
content = f.read()
f.close()

match = re.findall(r"{}(.*){}(.*)\n".format("CONFIG_SPIFFS_", "="), content)
config = {}
for item in match:
    v = item[1]
    if v == "y":
        v = "1"
    config["SPIFFS_"+item[0]] = v


flags =  ""
for key in config.keys():
    flags += ' -D'+key+'='+config[key]

flags += ' -DSPIFFS_READ_ONLY=0 -DSPIFFS_ALIGNED_OBJECT_INDEX_TABLES=1 -DSPIFFS_SINGLETON=0 -DSPIFFS_HAL_CALLBACK_EXTRA=0'

gen_exe_cmd = 'cd mkspiffs && make clean && make dist CPPFLAGS="'+flags+'" BUILD_CONFIG_NAME=-maixpy'
print("--------------------")
print(gen_exe_cmd)
print("--------------------")
ret = os.system(gen_exe_cmd)
if ret != 0:
    exit(1)



if not os.path.exists(out_dir):
    os.mkdir(out_dir)

dirs = os.listdir("mkspiffs")
exe_path = None
for dir in dirs:
    if os.path.isfile(os.path.join("mkspiffs", dir)):
        continue
    match = re.findall(r"{}(.*){}(.*)".format("mkspiffs", "maixpy"), dir)
    if len(match) > 0:
        exe_path = os.path.abspath("mkspiffs/"+dir+"/mkspiffs")
        break
print(exe_path)
logical_block_size = int(config["SPIFFS_LOGICAL_BLOCK_SIZE"], 16)
logical_page_size  = int(config["SPIFFS_LOGICAL_PAGE_SIZE"], 16)
image_size = int(config["SPIFFS_SIZE"], 16)
flash_addr = int(config["SPIFFS_START_ADDR"], 16)
gen_image_cmd = "{} -c {} -b {} -p {} -s {} {}/{}".format(exe_path, in_dir, logical_block_size, logical_page_size, image_size, out_dir, out_name)
print("--------------------")
print(gen_image_cmd)
print("--------------------")
ret = os.system(gen_image_cmd)
if ret != 0:
    exit(1)
print("[SUCCESS] image file in "+out_dir)
print("[USAGE] Please flash {}/{} to address 0x{:X} of flash with flash tool kflash_gui".format(out_dir, out_name, flash_addr))

