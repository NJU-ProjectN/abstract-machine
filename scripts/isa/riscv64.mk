CROSS_COMPILE := riscv64-linux-gnu-
COMMON_FLAGS  := -fno-pic -march=rv64g -mcmodel=medany
CFLAGS        += $(COMMON_FLAGS) -static
ASFLAGS       += $(COMMON_FLAGS) -O0
LDFLAGS       += -melf64lriscv
