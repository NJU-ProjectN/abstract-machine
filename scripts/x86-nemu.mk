include $(AM_HOME)/scripts/isa/x86.mk
include $(AM_HOME)/scripts/platform/nemu.mk
CFLAGS += -mstringop-strategy=loop
LDFLAGS += --defsym=_pmem_start=0x0
