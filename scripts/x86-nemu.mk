include $(AM_HOME)/scripts/isa/x86.mk
include $(AM_HOME)/scripts/platform/nemu.mk
CFLAGS  += -mstringop-strategy=loop -DISA_H=\"x86/x86.h\"
# overwrite _pmem_start and _entry_offset defined in nemu.mk
LDFLAGS += --defsym=_pmem_start=0x0 --defsym=_entry_offset=0x100000

AM_SRCS += x86/nemu/start.S \
           x86/nemu/cte.c \
           x86/nemu/trap.S \
           x86/nemu/vme.c
