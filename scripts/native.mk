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

# Some sdl2-config versions inject -D_GNU_SOURCE=1.  With glibc >= 2.43 that
# enables ISO C23 features (e.g. the type-generic strchr/strrchr macros) and
# also clashes with sources that define _GNU_SOURCE themselves.  Drop it.
SDL_CFLAGS := $(filter-out -D_GNU_SOURCE%, $(shell sdl2-config --cflags))
CFLAGS  += -fpie $(SDL_CFLAGS)
ASFLAGS += -fpie -pie
comma = ,
LDFLAGS_CXX = $(addprefix -Wl$(comma), $(LDFLAGS)) -pie -ldl $(shell sdl2-config --libs)

run: image
	$(IMAGE).elf

gdb: image
	gdb -ex "handle SIGUSR1 SIGUSR2 SIGSEGV noprint nostop" $(IMAGE).elf
