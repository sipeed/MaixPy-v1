import sys
import os

class LS:

    def __repr__(self):
        self.__call__()
        return ""

    def __call__(self, path=None):
        if path is None or path == ".":
            l = os.ls()
        else:
            l = os.ls(path)
        for f in l:
            print(f)

class PWD:

    def __repr__(self):
        res = os.getcwd()
        if res == "": # TLD on esp8266
            res = "/"
        return res

    def __call__(self):
        return self.__repr__()

class CLEAR:

    def __repr__(self):
        return "\x1b[2J\x1b[H"

    def __call__(self):
        return self.__repr__()
        
class MAN:

    def __repr__(self):
        return ("""
This is the 'upysh' command list:

ls
cat(file)
hd(file)
head(file [, #_of_lines])
man
mv(from, to)
newfile(file)
rm(file)
cd(dir)
pwd
""")

    def __call__(self):
        return self.__repr__()


ls = LS()
clear = CLEAR()
man = MAN()
mv = os.rename
rm = os.remove
cd = os.chdir
pwd = PWD()

def head(name, count=10):
    f = open(name)
    l = f.read().split("\n")
    n = 0
    for _ in l:
        print(_)
        n += 1
        if n >= count:
            break
    f.close()

def cat(f):
    head(f, 1 << 30)

#
# Hexdump Function
#
FILTER=''.join([(len(repr(chr(x)))==3) and chr(x) or '.' for x in range(256)])

def hexdump(src):
    N = 0
    result = ''
    length = 16
    while src:  # loop through string
        s,src = src[:length],src[length:]
        if type(s) is str:
            hexa = ' '.join(["{:02x}".format(ord(_)) for _ in s])
            s = ''.join([FILTER[ord(_)] for _ in s])
        else:
            hexa = ' '.join(["{:02x}".format(_) for _ in s])
            s = ''.join([FILTER[_] for _ in s])
        print("{:04x}   {:48s} {:s}".format(N, hexa, s))
        N += length

def hd(name):
    f = open(name)
    l = f.read()
    f.close()
    hexdump(l)

def newfile(path):
    print("Type file contents line by line, finish with EOF (Ctrl+D).")
    f = open(path, "w")
    data = ""
    while 1:
        try:
            l = input()
        except EOFError:
            break
        data += l + '\n'
    f.write(data)
    f.close()

