#!/usr/bin/python3

import argparse
import mmap

parser = argparse.ArgumentParser(description='Convert binary data to hexadecimal string (stdout): 8 bytes -> hex + \n')
parser.add_argument('input', metavar='FILE', type=str, help='path to input file')
args = parser.parse_args()
with open(args.input, "r+b") as f:
    m = mmap.mmap(f.fileno(), 0)
    while True:
        b = m.read(8)
        if b == b'':
            break
        print('0x{:016x}'.format(int.from_bytes(b, "big", signed=False)))
    m.close()

