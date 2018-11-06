#!/usr/bin/py
#!env python3
import time
import sys
import zlib
import os
from enum import Enum
import struct
import binascii
import hashlib
from Crypto.Cipher import AES
import argparse
aes_cipher_flag= b'\x00' 

firmware_len = 510*1024

micropython = open('micropython.bin','rb')
micropython_len = os.path.getsize('micropython.bin')
print("mpy len = %d",micropython_len)
micropython_buf=micropython.read()

dump_buf=bytearray(510*1024-micropython_len)
print("dump_buf len = ",len(dump_buf))

burn_data=aes_cipher_flag+struct.pack('I', firmware_len)+micropython_buf+dump_buf

sha256_hash = hashlib.sha256(burn_data).digest()

print(sha256_hash)

burn_data_with_sha256=burn_data+sha256_hash

micropython_new=open('micropython_sha256.bin','wb')
micropython_new.write(burn_data_with_sha256)
micropython_new.close()



