CROSS_COMPILE := riscv64-linux-gnu-
COMMON_FLAGS  := -fno-pic -march=rv64ifd -mcmodel=medany
CFLAGS        += $(COMMON_FLAGS) -static
ASFLAGS       += $(COMMON_FLAGS) -O0
LDFLAGS       += -melf64lriscv

AM_SRCS := mycpu/boot/start.S \
           mycpu/trm.c \
		   mycpu/ioe/timer.c \
		   mycpu/ioe/ioe.c \
		   mycpu/ioe/input.c

CFLAGS    += -fdata-sections -ffunction-sections
LDFLAGS   += -T $(AM_HOME)/am/src/mycpu/scripts/section.ld
LDFLAGS   += --gc-sections -e _start
CFLAGS += -DMAINARGS=\"$(mainargs)\"
CFLAGS += -I$(AM_HOME)/am/src/mycpu/include

image: $(IMAGE).elf
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin
