.global __am_kcontext_start
__am_kcontext_start:
#ifdef __x86_64__
  // rdi = arg, rsi = entry

  // (rsp + 8) should be multiple of 16 when
  // control is transfered to the function entry point.
  // See amd64 ABI manual for more details
  andq $0xfffffffffffffff0, %rsp
  call *%rsi
  call __am_panic_on_return
#elif __aarch64__
  // x0 = arg, x1 = entry
  // (sp + 16) should be multiple of 16 when
  // control is transfered to the function entry point.
  // See aarch64 ABI manual for more details
  // https://github.com/ARM-software/abi-aa
  mov x2, sp
  and sp, x2, #0xfffffffffffffff0
  br x1
  bl __am_panic_on_return
#elif __riscv
  // See riscv ABI manual for more details
  // https://github.com/riscv-non-isa/riscv-elf-psabi-doc
  addi sp, sp, -16
  andi sp, sp, ~15
  jalr a1
  jal  __am_panic_on_return
#endif
