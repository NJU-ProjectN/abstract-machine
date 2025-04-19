#!/usr/bin/env python3

from sys import argv

bin = argv[1]

f = open(bin + '-logisim-hex', 'w')
f.write('v3.0 hex words addressed\n')

i = 0
with open(bin, 'rb') as fbin:
  while True:
    byte4 = fbin.read(4)
    if not byte4:
      break
    if i % 8 == 0:
      f.write("%05x:" % i)
    f.write(" %08x" % int.from_bytes(byte4, "little"))
    if i % 8 == 7:
      f.write("\n")
    i = i + 1

fbin.close()
f.close()
