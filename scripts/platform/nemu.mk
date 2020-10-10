AM_SRCS := nemu/trm.c \
           nemu/ioe/ioe.c \
           nemu/ioe/timer.c \
           nemu/ioe/input.c \
           nemu/ioe/gpu.c \
           nemu/ioe/audio.c \
           nemu/isa/$(ISA)/cte.c \
           nemu/isa/$(ISA)/trap.S \
           nemu/isa/$(ISA)/vme.c \
           nemu/mpe.c \
           nemu/isa/$(ISA)/boot/start.S

CFLAGS    += -fdata-sections -ffunction-sections
LDFLAGS   += -L $(AM_HOME)/am/src/nemu/scripts
LDFLAGS   += -T $(AM_HOME)/am/src/nemu/isa/$(ISA)/boot/loader.ld
LDFLAGS   += --gc-sections -e _start
NEMUFLAGS += -b -l $(shell dirname $(IMAGE).elf)/nemu-log.txt $(IMAGE).bin

CFLAGS += -DMAINARGS=\"$(mainargs)\"
CFLAGS += -I$(AM_HOME)/am/src/nemu/include
.PHONY: $(AM_HOME)/am/src/nemu/trm.c

image: $(IMAGE).elf
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin

run: image
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMUFLAGS)"

gdb: image
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) gdb ARGS="$(NEMUFLAGS)"
