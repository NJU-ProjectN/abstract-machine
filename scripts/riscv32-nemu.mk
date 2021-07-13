include $(AM_HOME)/scripts/isa/riscv32.mk
include $(AM_HOME)/scripts/platform/nemu.mk
LDFLAGS += --defsym=_pmem_start=0x80000000
