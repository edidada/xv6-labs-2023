.text
.globl abisum
// int abisum(int n): preserve the return address across a nested call.
abisum:
  addi sp, sp, -16
  sd ra, 8(sp)
  call sum_to
  slli a0, a0, 1
  ld ra, 8(sp)
  addi sp, sp, 16
  ret

sum_to:
  mv t0, a0
  li a0, 0
1:
  add a0, a0, t0
  addi t0, t0, -1
  bgtz t0, 1b
  ret
