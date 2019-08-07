import os, sys, re

compile_path = os.path.dirname(sys.argv[0])
project_path = os.path.abspath(compile_path+"/..")
flash_tool_path = os.path.abspath(project_path+"/../../tools/flash")

with open(project_path+"/CMakeLists.txt") as f:
    content = f.read()
    match = re.findall(r"{}(.*){}".format("project\(", "\)"), content)
    project_name = match[0]

pack_kfpkg = False

firmware = project_path+"/build/"+project_name+".bin"
firmware_flash = project_path+"/build/"+project_name+"_flash.bin"
firmware_final = project_path+"/build/"+project_name+".kfpkg"

with open(project_path+"/build/config/global_config.mk") as f:
    content = f.read()
    match = re.findall(r"{}(.*)\n".format("CONFIG_FIRMWARE_FLASH_ADDR="), content)
    if len(match) >= 1:
        flash_firmware_addr = int(match[0], 16)
        pack_kfpkg = True

sys.path.append(flash_tool_path)
from kfpkg import KFPKG

if pack_kfpkg:
    kfpkg = KFPKG()
    kfpkg.addFile(0, firmware, True)
    kfpkg.addFile(flash_firmware_addr, firmware_flash, False) # TODO: add firmware check
    kfpkg.save(firmware_final)

    firmware = firmware_final

