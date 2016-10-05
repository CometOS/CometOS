#!/usr/bin/python
import struct;
import argparse;

def twos_comp(val, bits):
    """compute the 2's compliment of int value val"""
    if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is

parser = argparse.ArgumentParser()
parser.add_argument("id", type=str, help="NodeId to be flashed")

args = parser.parse_args()
id = int(args.id, 0)
assert(id > 0 and id < 65535)
chksum = ((((0x02) + (id & 0xFF) + (id >> 8)) & 0xFF) ^ 0xFF) + 0x01
data = ":02000000{0:2x}{1:2x}{2:2x}".format(id & 0xFF, id >> 8, chksum)
print data
