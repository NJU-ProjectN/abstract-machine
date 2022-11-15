include $(AM_HOME)/scripts/isa/loongarch32r.mk
include $(AM_HOME)/scripts/platform/nemu.mk
CFLAGS  += -DISA_H=\"loongarch/loongarch32r.h\"

AM_SRCS += loongarch/nemu/start.S \
           loongarch/nemu/cte.c \
           loongarch/nemu/trap.S \
           loongarch/nemu/vme.c
