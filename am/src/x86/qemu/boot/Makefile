SRCS := start.S main.c
bootblock.o: $(SRCS) Makefile
	@echo + CC $(SRCS)
	@$(CROSS_COMPILE)gcc -ggdb -static -m32 -fno-pic -Os -nostdlib -Ttext 0x7c00 -I$(AM_HOME)/am/src -o bootblock.o $(SRCS)
	# fix it with https://www.bilibili.com/read/cv34566265/?spm_id_from=333.999.0.0&jump_opus=1
	cp bootblock.o boot.o
	@python3 genboot.py bootblock.o

clean:
	rm -rf *.o
