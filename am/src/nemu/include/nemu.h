#ifndef NEMU_H__
#define NEMU_H__

#include <klib-macros.h>

#include ISA_H // "x86.h", "mips32.h", ...

#if defined(__ISA_X86__)
# define nemu_trap(code) asm volatile (".byte 0xd6" : :"a"(code))
#elif defined(__ISA_MIPS32__)
# define nemu_trap(code) asm volatile ("move $v0, %0; .word 0xf0000000" : :"r"(code))
#elif defined(__ISA_RISCV32__)
# define nemu_trap(code) asm volatile("mv a0, %0; .word 0x0000006b" : :"r"(code))
#elif
# error unsupported ISA __ISA__
#endif

#ifdef __ARCH_X86_NEMU
# define SERIAL_PORT  0x3f8
# define KBD_ADDR     0x60
# define RTC_ADDR     0x48
# define SCREEN_ADDR  0x100
# define AUDIO_ADDR   0x200
#else
# define SERIAL_PORT  0xa10003f8
# define KBD_ADDR     0xa1000060
# define RTC_ADDR     0xa1000048
# define SCREEN_ADDR  0xa1000100
# define AUDIO_ADDR   0xa1000200
#endif

#define FB_ADDR         0xa0000000
#define AUDIO_SBUF_ADDR 0xa0800000

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)
#define NEMU_PADDR_SPACE \
  RANGE(&_pmem_start, PMEM_END), \
  RANGE(FB_ADDR, FB_ADDR + 0x200000), \
  RANGE(0xa1000000, 0xa1000000 + 0x1000) /* serial, rtc, screen, keyboard */

typedef uintptr_t PTE;

#define PGSIZE    4096

#endif
