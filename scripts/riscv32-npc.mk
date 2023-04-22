include $(AM_HOME)/scripts/isa/riscv32.mk
CFLAGS += -march=rv32i

AM_SRCS := riscv/npc/start.S \
           riscv/npc/trm.c \
           riscv/npc/libgcc/div.S \
           riscv/npc/libgcc/muldi3.S

CFLAGS    += -fdata-sections -ffunction-sections
LDFLAGS   += -T $(AM_HOME)/scripts/linker.ld --defsym=_pmem_start=0x0 --defsym=_entry_offset=0x0
LDFLAGS   += --gc-sections -e _start
CFLAGS += -DMAINARGS=\"$(mainargs)\"
.PHONY: $(AM_HOME)/am/src/riscv/npc/trm.c

image: $(IMAGE).elf
	@$(OBJDUMP) -D $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin
	@truncate -s "%4" $(IMAGE).bin
	@python $(AM_HOME)/scripts/gen-logisim.py $(IMAGE).bin
