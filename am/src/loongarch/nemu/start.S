.section entry, "ax"
.globl _start
.type _start, @function

_start:
  move $fp, $zero
  la.local $sp, _stack_pointer
  bl _trm_init
