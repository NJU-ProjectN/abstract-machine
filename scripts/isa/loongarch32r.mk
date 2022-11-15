CROSS_COMPILE := loongarch32r-linux-gnusf-
COMMON_FLAGS  := -fno-pic
CFLAGS        += $(COMMON_FLAGS) -static
ASFLAGS       += $(COMMON_FLAGS) -O0
