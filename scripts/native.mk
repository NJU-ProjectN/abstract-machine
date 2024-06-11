AM_SRCS := native/trm.c \
           native/ioe.c \
           native/cte.c \
           native/trap.S \
           native/vme.c \
           native/mpe.c \
           native/platform.c \
           native/ioe/input.c \
           native/ioe/timer.c \
           native/ioe/gpu.c \
           native/ioe/uart.c \
           native/ioe/audio.c \
           native/ioe/disk.c \

CFLAGS  += -fpie $(shell sdl2-config --cflags)
ASFLAGS += -fpie -pie
comma = ,
LDFLAGS_CXX = $(addprefix -Wl$(comma), $(LDFLAGS)) -pie -ldl $(shell sdl2-config --libs)

run: image
	$(IMAGE).elf

gdb: image
	gdb -ex "handle SIGUSR1 SIGUSR2 SIGSEGV noprint nostop" $(IMAGE).elf
