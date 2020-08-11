AM_SRCS := native/trm.c \
           native/ioe.c \
           native/cte.c \
           native/trap.S \
           native/vme.c \
           native/mpe.c \
           native/platform.c \
           native/native-input.c \
           native/native-timer.c \
           native/native-gpu.c \
           native/native-audio.c \

CFLAGS  += -fpie
ASFLAGS += -fpie -pie

image:
	@echo + LD "->" $(IMAGE_REL)
	@g++ -pie -o $(IMAGE) -Wl,--whole-archive $(LINKAGE) -Wl,-no-whole-archive -lSDL2

run: image
	$(IMAGE)

gdb: image
	gdb -ex "handle SIGUSR1 SIGSEGV noprint nostop" $(IMAGE)
