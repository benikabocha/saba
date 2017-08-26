#!/usr/bin/env python

import sys
import zipfile

def extract(filename):
    with zipfile.ZipFile(filename) as zf:
        for zi in zf.infolist():
            zi.filename = zi.filename.encode('cp437').decode('cp932')
            zf.extract(zi)

if __name__ == '__main__':
    extract(sys.argv[1])

