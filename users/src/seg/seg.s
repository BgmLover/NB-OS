.globl seg_start
seg_start:
li $t0, 0x12345678
li $t1, 0xbfc09008
sw $t0, 0($t1)
nop
jr $ra
nop
