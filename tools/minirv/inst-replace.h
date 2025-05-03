#if 0
Base instructions:
* add, addi, lui, lw, lbu, sw, sb, jalr

Assumption:
* compiler do not use tp and gp
* stack below sp is not used yet

Calling convention:
* tp - temporary register
* gp - saved register, but temporary for the most outer functions

Limitation:
* We can not break sw into sb, since this process depends on shift operations.
  But shift operationss also depend on sw.
* It is difficult to break lw into lbu, since it requires at least two temporary
  registers. This makes it difficult to implement other instructions with lw,
  due to the small numbers of avaliable registers.
* We can not use lw/sw io impement lbu/sb, since this process requires detecting
  misaligned memory accessing.
* We can not break lui into addi + slli, since the linker can not provide
  symbol[31:20] and symbol[19:12]. The linker can only provide %hi(symbol)
  and %lo(symbol), which are symbol[31:12] and symbol[11:0] respectively.
#endif

#define _concat(x, y) x ## y
#define concat(x, y) _concat(x, y)
#define SET_DEBUG_LABEL(name) concat(name, _\@):

#define SP_VAR_BYTE(n, boffset) (-(n) * 4 + (boffset))(sp)
#define SP_VAR(n) SP_VAR_BYTE(n, 0)
#define VAR_A 4
#define VAR_B 5
#define VAR_C 6
#define VAR_D 7
#define PUSH(r, n) sw r, SP_VAR(n)
#define POP(r, n)  lw r, SP_VAR(n)

.section .rodata
.align 8
.weak _same_result_table
_same_result_table: .byte 1, 0, 0, 0, 0, 0, 0, 0

.weak _check_8bit_0_table
_check_8bit_0_table:
.byte 0
.fill 255, 1, 1

# slt = OF ^ SF = ((sign(A) ^ sign(B)) & (sign(A) ^ sign(R))) ^ sign(R)
# sltu = slt ^ sign(A) ^ sign(B)
# A  B  R  OF  slt  sltu (sltu = slt ^ A ^ B)
# 0  0  0  0    0    0
# 0  0  1  0    1    1
# 0  1  0  0    0    1
# 0  1  1  1    0    1
# 1  0  0  1    1    0
# 1  0  1  0    1    0
# 1  1  0  0    0    0
# 1  1  1  0    1    1
.weak _slt1_table
_slt1_table: .byte 0, 1, 0, 0, 1, 1, 0, 1
.weak _sltu1_table
_sltu1_table: .byte 0, 1, 1, 1, 0, 0, 0, 1

.weak _or1_table
_or1_table: .byte 0, 1, 1, 1

.weak _logic_shift_table
.align 8
_logic_shift_table:
.incbin _LUT_BIN_PATH  # defined in command line flags

#define _and8_table _logic_shift_table
#define _or8_table  (_and8_table + 256 * 256)
#define _xor8_table (_or8_table  + 256 * 256)
#define _sll8_table (_xor8_table + 256 * 256)
#define _srl8_table (_sll8_table + 32 * 256 * 4)
#define _sra8_table (_srl8_table + 32 * 256 * 4)

#define def_itype(name, rtype_name) \
  .macro name rd, rs1, imm ;\
    SET_DEBUG_LABEL(name); \
    li tp, \imm; \
    rtype_name \rd, \rs1, tp; \
  .endm

.section .data
.align 8
.weak _check_same_array
_check_same_array: .fill 256, 1, 0

.macro call_template r, addr
  la \r, \addr
  jalr \r, \r
.endm

.macro jal rd, addr
  SET_DEBUG_LABEL(jal)
  .if \rd == zero
    j \addr
  .else
  .if \rd == x0
    j \addr
  .else
    call_template \rd, \addr
  .endif
  .endif
.endm

.macro sub rd, rs1, rs2
  SET_DEBUG_LABEL(sub)
  not tp, \rs2
  add \rd, \rs1, tp
  addi \rd, \rd, 1
.endm

.macro getbyte rd, rs1, byte
  sw \rs1, SP_VAR(VAR_C)
  lbu \rd, SP_VAR_BYTE(VAR_C, \byte)
.endm

.macro msb rd, rs1
  getbyte \rd, \rs1, 3
  slli \rd, \rd, 1
  getbyte \rd, \rd, 1
.endm

.macro slt_template lut, rd, rs1, rs2
  PUSH(s0, 1)
  PUSH(s1, 2)
  sw \rs2, SP_VAR(VAR_A)
  mv s0, \rs1
  lw s1, SP_VAR(VAR_A)

  msb gp, s0
  slli gp, gp, 1

  msb tp, s1
  add gp, gp, tp
  slli gp, gp, 1

  sub tp, s0, s1 # sub uses tp
  msb tp, tp
  add gp, gp, tp

  la tp, \lut
  add tp, tp, gp

  POP(s1, 2)
  POP(s0, 1)

  lbu \rd, (tp)
.endm

.macro slt rd, rs1, rs2
  SET_DEBUG_LABEL(slt)
  slt_template _slt1_table, \rd, \rs1, \rs2
.endm

.macro sltu rd, rs1, rs2
  SET_DEBUG_LABEL(slt)
  slt_template _sltu1_table, \rd, \rs1, \rs2
.endm

.macro logic8_byte_internal lut_reg, boffset
  lbu tp, SP_VAR_BYTE(VAR_A, \boffset)
  sb tp, SP_VAR_BYTE(VAR_D, 1)
  lbu tp, SP_VAR_BYTE(VAR_B, \boffset)
  sb tp, SP_VAR_BYTE(VAR_D, 0)
  lw tp, SP_VAR(VAR_D)
  add tp, \lut_reg, tp
  lbu tp, (tp)
  sb tp, SP_VAR_BYTE(VAR_C, \boffset)
.endm

.macro logic lut, rd, rs1, rs2
  .if \rd == x0
    .exitm
  .endif
  .if \rd == sp || \rs1 == sp || \rs2 == sp
    .abort
  .endif
  .if \rd == gp || \rs1 == gp || \rs2 == gp
    .abort
  .endif
  .if \rd == tp || \rs1 == tp
    .abort
  .endif
  sw \rs1, SP_VAR(VAR_A)
  sw \rs2, SP_VAR(VAR_B)
  la \rd, \lut
  sw x0, SP_VAR(VAR_D)

  logic8_byte_internal \rd, 3
  logic8_byte_internal \rd, 2
  logic8_byte_internal \rd, 1
  logic8_byte_internal \rd, 0

  lw \rd, SP_VAR(VAR_C)
.endm

#define def_logic(name) \
  .macro name rd, rs1, rs2 ;\
    SET_DEBUG_LABEL(name); \
    logic concat(_, concat(name, 8_table)), \rd, \rs1, \rs2; \
  .endm

def_logic(and)
def_logic(or)
def_logic(xor)


.macro check_8bit_same table, rd, boffset  # rd == 0 if same
  lbu \rd, SP_VAR_BYTE(VAR_A, \boffset)
  add tp, \table, \rd
  addi \rd, x0, 1
  sb \rd, (tp)

  lbu \rd, SP_VAR_BYTE(VAR_B, \boffset)
  add \rd, \table, \rd
  sb x0, (\rd)

  lbu \rd, (tp)
.endm

.macro seq rd, rs1, rs2   # set equal
  sw \rs1, SP_VAR(VAR_A)
  sw \rs2, SP_VAR(VAR_B)
  PUSH(s0, 1)
  PUSH(s1, 2)

  la s0, _check_same_array
  check_8bit_same s0, gp, 0
  check_8bit_same s0, s1, 1
  add gp, gp, s1
  check_8bit_same s0, s1, 2
  add gp, gp, s1
  check_8bit_same s0, s1, 3
  add gp, gp, s1

  la tp, _same_result_table
  add tp, tp, gp
  lbu \rd, (tp)

  POP(s1, 2)
  POP(s0, 1)
.endm

.macro check_8bit_0 table, rd, boffset  # rd = (source is 0 ? 0 : 1)
  lbu \rd, SP_VAR_BYTE(VAR_A, \boffset)
  add \rd, \table, \rd
  lbu \rd, (\rd)
.endm

.macro seqz rd, rs1   # set if 0
  sw \rs1, SP_VAR(VAR_A)
  PUSH(s0, 1)

  la s0, _check_8bit_0_table
  check_8bit_0 s0, gp, 0
  check_8bit_0 s0, tp, 1
  add gp, gp, tp
  check_8bit_0 s0, tp, 2
  add gp, gp, tp
  check_8bit_0 s0, tp, 3
  add gp, gp, tp

  la tp, _same_result_table
  add tp, tp, gp
  lbu \rd, (tp)

  POP(s0, 1)
.endm

#define SET_BRANCH_LABEL(name) concat(name, _\@\target):

.macro branch_resolve target, is_tp_high_sel_target, label
#define LABEL_NAME non_taken_\label\target
  slli tp, tp, 2  # tp = tp * 4
  add  tp, sp, tp
  .if \is_tp_high_sel_target == 1
    la gp, \target
    sw gp, SP_VAR(VAR_A)  # higher address
    la gp, LABEL_NAME
    sw gp, SP_VAR(VAR_B)  # lower address
  .else
    la gp, LABEL_NAME
    sw gp, SP_VAR(VAR_A)  # higher address
    la gp, \target
    sw gp, SP_VAR(VAR_B)  # lower address
  .endif
  lw tp, (-VAR_B * 4)(tp)
  jr tp

LABEL_NAME:
.endm


.macro beq rs1, rs2, target
  SET_BRANCH_LABEL(beq)
  .if \rs2 == zero || \rs2 == x0
    seqz tp, \rs1
  .else
    .if \rs1 == zero || \rs1 == x0
      beq \rs2, \rs1, \target
      .exitm
    .else
      seq tp, \rs1, \rs2
    .endif
  .endif
  branch_resolve \target, 1, \@
.endm

.macro bne rs1, rs2, target
  SET_BRANCH_LABEL(bne)
  .if \rs2 == zero || \rs2 == x0
    seqz tp, \rs1
  .else
    .if \rs1 == zero || \rs1 == x0
      bne \rs2, \rs1, \target
      .exitm
    .else
      seq tp, \rs1, \rs2
    .endif
  .endif
  branch_resolve \target, 0, \@
.endm

.macro blt rs1, rs2, target
  SET_BRANCH_LABEL(blt)
  .if \rs2 == zero || \rs2 == x0
    msb tp, \rs1
    branch_resolve \target, 1, \@
  .else
    .if \rs1 == zero || \rs1 == x0
      # check rs2 > 0 ?
      # msb(rs2) == 0 && msb(rs2 - 1) == 0
      msb gp, \rs2
      addi tp, \rs2, -1
      msb tp, tp
      add gp, gp, tp
      la tp, _or1_table
      add tp, gp, tp
      lbu tp, (tp)  # tp == 1 -> not taken
      branch_resolve \target, 0, \@
    .else
      slt tp, \rs1, \rs2
      branch_resolve \target, 1, \@
    .endif
  .endif
.endm

.macro bltu rs1, rs2, target
  SET_BRANCH_LABEL(bltu)
  sltu tp, \rs1, \rs2
  branch_resolve \target, 1, \@
.endm

.macro bge rs1, rs2, target
  SET_BRANCH_LABEL(bge)
  .if \rs1 == zero || \rs1 == x0
    # check rs2 <= 0 ?
    # msb(rs2) == 1 || msb(rs2 - 1) == 1
    msb gp, \rs2
    addi tp, \rs2, -1
    msb tp, tp
    add gp, gp, tp
    la tp, _or1_table
    add tp, gp, tp
    lbu tp, (tp)
    branch_resolve \target, 1, \@
  .else
    slt tp, \rs1, \rs2
    branch_resolve \target, 0, \@
  .endif
.endm

.macro bgeu rs1, rs2, target
  SET_BRANCH_LABEL(bgeu)
  sltu tp, \rs1, \rs2
  branch_resolve \target, 0, \@
.endm

.macro getshamt rd, rs1
  sb \rs1, SP_VAR_BYTE(VAR_B, 0)
  lbu \rd, SP_VAR_BYTE(VAR_B, 0)
  la tp, (_and8_table + (0x1f << 8))
  add \rd, \rd, tp
  lbu \rd, (\rd)
.endm

.macro get_shift_byte_in_tp base, boffset
  lbu tp, SP_VAR_BYTE(VAR_A, \boffset)
  slli tp, tp, 2 # word per entry
  add tp, \base, tp
  lw tp, (tp)
.endm

.macro sll rd, rs1, rs2
  SET_DEBUG_LABEL(sll)
  PUSH(s0, 1)

  sw \rs1, SP_VAR(VAR_A)
  getshamt s0, \rs2
  sw x0, SP_VAR(VAR_B)
  sb s0, SP_VAR_BYTE(VAR_B, 1)
  lw s0, SP_VAR(VAR_B)  # s0 = s0 << 8
  slli s0, s0, 2  # word per entry
  la tp, _sll8_table
  add s0, s0, tp

  sw x0, SP_VAR(VAR_B)
  get_shift_byte_in_tp s0, 3
  sb tp, SP_VAR_BYTE(VAR_B, 3)
  lw gp, SP_VAR(VAR_B)  # gp = tp << 24

  get_shift_byte_in_tp s0, 2
  sh tp, SP_VAR_BYTE(VAR_B, 2)
  lw tp, SP_VAR(VAR_B)  # tp = tp << 16
  add gp, gp, tp

  get_shift_byte_in_tp s0, 1
  slli tp, tp, 8
  add gp, gp, tp

  get_shift_byte_in_tp s0, 0

  POP(s0, 1)
  add \rd, gp, tp
.endm

.macro shift_right_template rd, rs1, rs2, is_arith, is_rs2_imm
  .if \is_rs2_imm == 1 && \rs2 == 24  # fast path
    .if \is_arith == 1
      sw \rs1, SP_VAR(VAR_A)
      la tp, (_sra8_table + (\rs2 << 10))
      lbu \rd, SP_VAR_BYTE(VAR_A, 3)
      slli \rd, \rd, 2 # word per entry
      add tp, tp, \rd
      lw \rd, (tp)
    .else
      getbyte \rd, \rs1, 3
    .endif
    .exitm
  .endif


  PUSH(s0, 1)

  sw \rs1, SP_VAR(VAR_A)
  .if \is_rs2_imm == 0
    getshamt s0, \rs2
    sw x0, SP_VAR(VAR_B)
    sb s0, SP_VAR_BYTE(VAR_B, 1)
    lw s0, SP_VAR(VAR_B)  # s0 = s0 << 8
    slli s0, s0, 2  # word per entry
    la tp, _srl8_table
    add s0, s0, tp
  .else
    la s0, (_srl8_table + (\rs2 << 10))
  .endif

  # result = ((x3 << 24) | (x2 << 16) | (x1 << 8) | x0) >> a
  # let y be the values fetched from the table, then
  #   y3 = (x3 << 24 >> a)
  #   y2 = (x2 << 24 >> a)
  #   y1 = (x1 << 24 >> a)
  #   y0 = (x0 << 24 >> a)
  # result = y3 | (y2 >> 8) | (y1 >> 16) | (y0 >> 24)
  #        = { y33, y32, y31, y30 } |
  #               { y23, y22, y21 } |
  #                    { y13, y12 } |
  #                         { y03 }

  get_shift_byte_in_tp s0, 2
  sw tp, SP_VAR(VAR_B)
  get_shift_byte_in_tp s0, 1
  sw tp, SP_VAR(VAR_C)
  get_shift_byte_in_tp s0, 0
  sw tp, SP_VAR(VAR_D)

  # y3 is added in the end
  # compute byte 0
  lbu gp, SP_VAR_BYTE(VAR_B, 1)
  lbu tp, SP_VAR_BYTE(VAR_C, 2)
  add gp, gp, tp
  lbu tp, SP_VAR_BYTE(VAR_D, 3)
  add gp, gp, tp
  sw gp, SP_VAR(VAR_D)  # no carry during addition

  # compute byte 1
  lbu gp, SP_VAR_BYTE(VAR_B, 2)
  lbu tp, SP_VAR_BYTE(VAR_C, 3)
  add gp, gp, tp
  sb gp, SP_VAR_BYTE(VAR_D, 1)  # no carry during addition

  # compute byte 2
  lbu gp, SP_VAR_BYTE(VAR_B, 3)
  sb gp, SP_VAR_BYTE(VAR_D, 2)

  # combine byte 2~0
  lw gp, SP_VAR(VAR_D)

  .if \is_arith == 1
    la tp, (_sra8_table - _srl8_table)
    add s0, s0, tp
  .endif

  get_shift_byte_in_tp s0, 3

  POP(s0, 1)
  add \rd, gp, tp
.endm

.macro srl rd, rs1, rs2
  SET_DEBUG_LABEL(srl)
  shift_right_template \rd, \rs1, \rs2, 0, 0
.endm

.macro sra rd, rs1, rs2
  SET_DEBUG_LABEL(sra)
  shift_right_template \rd, \rs1, \rs2, 1, 0
.endm


.macro slli rd, rs1, imm
  .if \imm == 0
    .if \rd != \rs1
      mv \rd, \rs1
    .endif
    .exitm
  .endif
  .if \imm >= 24
    sw x0, SP_VAR(VAR_A)
    sb \rs1, SP_VAR_BYTE(VAR_A, 3)
    lw \rd, SP_VAR(VAR_A)
    slli \rd, \rd, (\imm - 24)
    .exitm
  .endif
  .if \imm >= 16
    sw x0, SP_VAR(VAR_A)
    sh \rs1, SP_VAR_BYTE(VAR_A, 2)
    lw \rd, SP_VAR(VAR_A)
    slli \rd, \rd, (\imm - 16)
    .exitm
  .endif
  SET_DEBUG_LABEL(slli)
  add \rd, \rs1, \rs1
  .rept (\imm - 1)
    add \rd, \rd, \rd
  .endr
.endm

.macro srli rd, rs1, imm
  SET_DEBUG_LABEL(srli)
  shift_right_template \rd, \rs1, \imm, 0, 1
.endm

.macro srai rd, rs1, imm
  SET_DEBUG_LABEL(srai)
  shift_right_template \rd, \rs1, \imm, 1, 1
.endm

def_itype(slti, slt)
def_itype(sltiu, sltu)
def_itype(andi, and)
def_itype(ori, or)
def_itype(xori, xor)

.macro lb rd, addr
  sw x0, SP_VAR(VAR_A)
  lbu \rd, \addr
  sb \rd, SP_VAR_BYTE(VAR_A, 3)
  lw \rd, SP_VAR(VAR_A)
  srai \rd, \rd, 24
.endm

.macro lhu rd, addr
  sw x0, SP_VAR(VAR_A)
  lbu tp, 1 + \addr
  sb tp, SP_VAR_BYTE(VAR_A, 1)
  lw tp, SP_VAR(VAR_A)
  lbu \rd, 0 + \addr
  add \rd, \rd, tp
.endm

.macro lh rd, addr
  sw x0, SP_VAR(VAR_A)
  lbu gp, 1 + \addr
  sb gp, SP_VAR_BYTE(VAR_A, 3)
  lw gp, SP_VAR(VAR_A)
  srai gp, gp, 16
  lbu \rd, 0 + \addr
  add \rd, \rd, gp
.endm

.macro sh rs, addr
  sb \rs, 0 + \addr
  getbyte tp, \rs, 1
  sb tp, 1 + \addr
.endm

# RISC-V pseudo-instruction

.macro la r, addr
  lui \r, %hi(\addr)
  addi \r, \r, %lo(\addr)
.endm

.macro lla r, addr
  la \r, \addr
.endm

.macro call addr
  call_template ra, \addr
.endm

.macro tail addr
  la tp, \addr
  jr tp
  call_template ra, \addr
.endm

.macro j addr
  la tp, \addr
  jalr zero, tp
.endm

.macro not rd, rs1
  PUSH(gp, 3)
  sw \rs1, SP_VAR(VAR_C)
  la tp, (_xor8_table + (0xff << 8))

  lbu gp, SP_VAR_BYTE(VAR_C, 3)
  add gp, gp, tp
  lbu gp, (gp)
  sb  gp, SP_VAR_BYTE(VAR_D, 3)

  lbu gp, SP_VAR_BYTE(VAR_C, 2)
  add gp, gp, tp
  lbu gp, (gp)
  sb  gp, SP_VAR_BYTE(VAR_D, 2)

  lbu gp, SP_VAR_BYTE(VAR_C, 1)
  add gp, gp, tp
  lbu gp, (gp)
  sb  gp, SP_VAR_BYTE(VAR_D, 1)

  lbu gp, SP_VAR_BYTE(VAR_C, 0)
  add gp, gp, tp
  lbu gp, (gp)
  sb  gp, SP_VAR_BYTE(VAR_D, 0)

  POP(gp, 3)

  lw \rd, SP_VAR(VAR_D)
.endm

.macro neg rd, rs
  sub \rd, x0, \rs
.endm

.macro snez rd, rs
  sltu \rd, x0, \rs
.endm

.macro sltz rd, rs
  msb \rd, \rs
.endm

.macro sgt rd, rs1, rs2
  slt \rd, \rs2, \rs1
.endm

.macro sgtu rd, rs1, rs2
  sltu \rd, \rs2, \rs1
.endm

.macro sgtz rd, rs
  slt \rd, x0, \rs
.endm

#if 0
# already defined above
.macro seqz rd, rs
  sltiu \rd, \rs, 1
.endm
#endif

.macro beqz rs, offset
  beq \rs, x0, \offset
.endm

.macro bnez rs, offset
  bne \rs, x0, \offset
.endm

.macro blez rs, offset
  bge x0, \rs, \offset
.endm

.macro bgez rs, offset
  bge \rs, x0, \offset
.endm

.macro bltz rs, offset
  blt \rs, x0, \offset
.endm

.macro bgtz rs, offset
  blt x0, \rs, \offset
.endm

.macro bgt rs, rt, offset
  blt \rt, \rs, \offset
.endm

.macro ble rs, rt, offset
  bge \rt, \rs, \offset
.endm

.macro bgtu rs, rt, offset
  bltu \rt, \rs, \offset
.endm

.macro bleu rs, rt, offset
  bgeu \rt, \rs, \offset
.endm

#undef _concat
#undef concat
#undef PUSH
#undef POP
