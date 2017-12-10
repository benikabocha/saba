#!/usr/bin/env python

import sys
import os

def bin2h(filename):
    with open(filename, "rb") as infile:
        data = infile.read()
        fname = os.path.basename(filename)
        with open(fname + ".h", "w") as outfile:
            outfile.write("static const uint8_t {0}_data[] = {{\n".format(fname.replace(".", "_")))
            for i in range(len(data)):
                if (i >= 1 and i % 16 == 0):
                    outfile.write('\n')
                outfile.write("{0:#04x},".format(data[i]))
            outfile.write("\n};")
        print()

if __name__ == '__main__':
    bin2h(sys.argv[1])
