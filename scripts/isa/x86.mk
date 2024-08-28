export CROSS_COMPILE := x86_64-linux-gnu-
CFLAGS  += -m32 -fno-pic -fno-omit-frame-pointer -march=i386
CFLAGS  += -fcf-protection=none # remove endbr32 in Ubuntu 20.04 with a CPU newer than Comet Lake
CFLAGS  += --param=min-pagesize=0 # fix warning about "array subscript 0 is outside array bounds"
ASFLAGS += -m32 -fno-pic
LDFLAGS += -melf_i386
