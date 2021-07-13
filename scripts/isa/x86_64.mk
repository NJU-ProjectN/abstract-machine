export CROSS_COMPILE := x86_64-linux-gnu-
CFLAGS  += -m64 -fPIC -mno-sse
ASFLAGS += -m64 -fPIC
LDFLAGS += -melf_x86_64
