.global __am_kcontext_start
__am_kcontext_start:
  // rdi = arg, rsi = entry

  // (rsp + 8) should be multiple of 16 when
  // control is transfered to the function entry point.
  // See amd64 ABI manual for more details
  andq $0xfffffffffffffff0, %rsp
  call *%rsi
  call __am_panic_on_return
