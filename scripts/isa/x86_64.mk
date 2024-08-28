export CROSS_COMPILE := x86_64-linux-gnu-
CFLAGS  += -m64 -fPIC -mno-sse
CFLAGS  += --param=min-pagesize=0 # fix warning about "array subscript 0 is outside array bounds"
ASFLAGS += -m64 -fPIC
LDFLAGS += -melf_x86_64
