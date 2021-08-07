#include <x86/x86.h>
#include "x86-qemu.h"

.code32
.globl _start
_start:
  movl  $(PDPT_ADDR | PTE_P | PTE_W), %eax
  cmpl  (PML4_ADDR), %eax
  je    .long_mode_init

  movl  $(PDPT_ADDR | PTE_P | PTE_W), %eax
  movl  %eax, (PML4_ADDR)

  movl  $0,   %ecx
  movl  $512, %esi                      //  512 pages
                                        //   |
.loop:                                  //   x
  movl  %ecx, %eax                      //   |
  shll  $30,  %eax                      //   |
  orl   $(PTE_P | PTE_W | PTE_PS), %eax // 1 GiB page
  movl  %eax, PDPT_ADDR(, %ecx, 8)

  movl  %ecx, %eax
  shrl  $2,   %eax
  movl  %eax, PDPT_ADDR + 4(, %ecx, 8)

  inc  %ecx
  cmp  %esi, %ecx
  jne  .loop

.long_mode_init:
  movl  $PML4_ADDR,  %eax
  movl  %eax,        %cr3     // %cr3 = PML4 base
  movl  $CR4_PAE,    %eax
  movl  %eax,        %cr4     // %cr4.PAE = 1
  movl  $0xc0000080, %ecx
  rdmsr
  orl   $0x100,      %eax
  wrmsr                       // %EFER.LME = 1
  movl  %cr0,        %eax
  orl   $CR0_PG,     %eax
  movl  %eax,        %cr0     // %cr0.PG = 1
  lgdt  gdt_ptr               // bootstrap GDT
  ljmp  $8, $_start64         // should not return

.code64
_start64:
  movw  $0,  %ax
  movw  %ax, %ds
  movw  %ax, %es
  movw  %ax, %ss
  movw  %ax, %fs
  movw  %ax, %gs

  movq  $MAINARG_ADDR, %rdi
  pushq $0
  jmp   _start_c

.align 16
gdt_ptr:
  .word gdt64_end - gdt64_begin - 1
  .quad gdt64_begin

gdt64_begin:
  .long 0x00000000  // 0: null desc
  .long 0x00000000
  .long 0x00000000  // 1: code
  .long 0x00209800
gdt64_end:
