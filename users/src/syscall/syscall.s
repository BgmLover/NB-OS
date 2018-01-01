.globl exec_start
exec_start:
la $a0, s
addi $v0, $zero, 4
syscall 4
nop
jr $ra
nop
s:
.asciiz "ZJUNIX exec test\n"
