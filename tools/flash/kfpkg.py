
import json, zipfile, os, tempfile

class KFPKG():
    def __init__(self):
        self.fileInfo = {"version": "0.1.0", "files": []}
        self.filePath = {}
        self.burnAddr = []
    
    def addFile(self, addr, path, prefix=False):
        if not os.path.exists(path):
            raise ValueError("FilePathError")
        if addr in self.burnAddr:
            raise ValueError("Burn dddr duplicate"+":0x%06x" %(addr))
        f = {}
        f_name = os.path.split(path)[1]
        f["address"] = addr
        f["bin"] = f_name
        f["sha256Prefix"] = prefix
        self.fileInfo["files"].append(f)
        self.filePath[f_name] = path
        self.burnAddr.append(addr)

    def listDumps(self):
        kfpkg_json = json.dumps(self.fileInfo, indent=4)
        return kfpkg_json

    def listDump(self, path):
        with open(path, "w") as f:
            f.write(json.dumps(self.fileInfo, indent=4))

    def listLoads(self, kfpkgJson):
        self.fileInfo = json.loads(kfpkgJson)

    def listLload(self, path):
        with open(path) as f:
            self.fileInfo = json.load(f)

    def save(self, path):
        listName = os.path.join(tempfile.gettempdir(), "kflash_gui_tmp_list.json")
        self.listDump(listName)
        try:
            with zipfile.ZipFile(path, "w") as zip:
                for name,path in self.filePath.items():
                    zip.write(path, arcname=name, compress_type=zipfile.ZIP_DEFLATED)
                zip.write(listName, arcname="flash-list.json", compress_type=zipfile.ZIP_DEFLATED)
                zip.close()
        except Exception as e:
            os.remove(listName)
            raise e
        os.remove(listName)

