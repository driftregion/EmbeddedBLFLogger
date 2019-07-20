#!/usr/bin/env python3

USAGE = """test.py [.blf file]"""

import sys
import ipdb


def main():
    try:
        filename = sys.argv[1]
    except IndexError:
        print(USAGE)
        return
    from can import BLFReader
    with ipdb.launch_ipdb_on_exception():
        reader = BLFReader(filename)
        for frame in reader:
            print(frame)
            ipdb.set_trace()

if __name__ == '__main__':
    main()
