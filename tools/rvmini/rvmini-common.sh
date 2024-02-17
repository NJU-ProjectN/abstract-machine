#!/bin/bash

src=${*: -1}
dst=${*: -2:1}
cc=$1
flags=${*: 2:$#-4}

dst_S=${dst%.o}.S
if [[ "$src" == *.S ]] then
  cp $src $dst_S
else
  riscv64-linux-gnu-$cc $flags -S -o $dst_S $src
fi

# replace pseudo instructions for load/store
sp="[[:space:]]*"
reg="[[:alnum:]]+"
comma="$sp,$sp"
symbol="[[:alnum:]_]+"
sed -E -i -e "s/(l[bhw]u?)${sp}(${reg})${comma}(${symbol})(${sp}[-+]${sp}${symbol})?${sp}\$/la \2, \3\4; \1 \2, 0(\2);/" \
          -e "s/(s[bhw])${sp}(${reg})${comma}(${symbol})(${sp}[-+]${sp}${symbol})?${comma}(${reg})${sp}\$/la \5, \3\4; \1 \2, 0(\5);/" $dst_S

# insert inst-replace.h to each .h files
rvmini_path=$AM_HOME/tools/rvmini
lut_bin_path=$rvmini_path/lut.bin
sed -i "1i#include \"$rvmini_path/inst-replace.h\"" $dst_S
flock $rvmini_path/.lock -c "test -e $lut_bin_path || (cd $rvmini_path && gcc gen-lut.c && ./a.out && rm a.out)"

riscv64-linux-gnu-gcc $flags -D_LUT_BIN_PATH=\"$lut_bin_path\" -Wno-trigraphs -c -o $dst $dst_S
