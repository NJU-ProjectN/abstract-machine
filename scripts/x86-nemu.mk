include $(AM_HOME)/scripts/isa/x86.mk
CFLAGS += -mstringop-strategy=loop
include $(AM_HOME)/scripts/platform/nemu.mk
