include $(AM_HOME)/scripts/isa/riscv64.mk

AM_SRCS := spike/trm.c \
           spike/ioe.c \
           spike/timer.c \
           spike/start.S \
           spike/htif.S \
           platform/dummy/cte.c \
           platform/dummy/vme.c \
           platform/dummy/mpe.c \

CFLAGS    += -fdata-sections -ffunction-sections
LDFLAGS   += -T $(AM_HOME)/am/src/spike/linker.ld
LDFLAGS   += --gc-sections -e _start

CFLAGS += -DMAINARGS=\"$(mainargs)\"
.PHONY: $(AM_HOME)/am/src/spike/trm.c

image: $(IMAGE).elf
