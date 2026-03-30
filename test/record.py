#!/usr/bin/env python3

import sys

if len(sys.argv) < 4:
    print(f"Usage: {sys.argv[0]} OUTPUT_FILE SENDER RECIPIENT...", file=sys.stderr)
    sys.exit(1)

with open(sys.argv[1], 'wb') as f:
    for i in sys.argv[2:]:
        f.write(i.encode('utf-8'))
        f.write(b'\n')
    f.write(b'\n')
    source = sys.stdin.buffer
    while True:
        data = source.read(65536)
        if data == b'':
            break
        f.write(data)
