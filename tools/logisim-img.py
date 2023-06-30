#!/usr/bin/env python3

from sys import argv

bin = argv[1]

fp_inst  = open(bin + '-logisim-inst.txt', 'w')
fp_data0 = open(bin + '-logisim-data0.txt', 'w')
fp_data1 = open(bin + '-logisim-data1.txt', 'w')
fp_data2 = open(bin + '-logisim-data2.txt', 'w')
fp_data3 = open(bin + '-logisim-data3.txt', 'w')
for f in [fp_inst, fp_data0, fp_data1, fp_data2, fp_data3]:
  f.write('v2.0 raw\n')

with open(bin, 'rb') as fp:
  while True:
    bytes = fp.read(4)
    if not bytes:
      break
    fp_inst.write(bytes[::-1].hex() + ' ')
    fp_data0.write("%02x " % bytes[0])
    fp_data1.write("%02x " % bytes[1])
    fp_data2.write("%02x " % bytes[2])
    fp_data3.write("%02x " % bytes[3])
  fp.close()

for f in [fp_inst, fp_data0, fp_data1, fp_data2, fp_data3]:
  f.close()
