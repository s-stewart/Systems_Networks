Multiply numbers by 4 for a byte addressed memory
this includes the instruction numbering in the left margin and
all offsets to memory or the stack pointer

If and only if two registers have the same name 
then they are assembled as the same register

the style is as in the book
$14 is the stack pointer
$15 is the return address register
$3 is just another register 
(if there were an assembler it would be the reserved for assembler)
$v# are return value registers
$a# are argument registers
$zero is the register which is hard wired to be 0

main:                            Imm (assuming 8 bit, please sign extend)
00        ldadr $14, initsp      0x29   ! initialize the stack pointer
01        lw $14, 0($14)                ! finish initialization
                
02        ldadr $1, BASE        0x08   !load base for pow
03        lw $1, 0($1)
04        ldadr $2, EXP         0x07   !load power for pow
05        lw $2, 0($2)
06        ldadr $3, POW         0x07   !load address of pow
07        jalr $3, $15                 !run pow
08        ldadr $1, ANS         0x04   !load base for pow
09        sw $10, 0($1)
                
0A        halt        
        
0B        BASE: .fill 2
0C        EXP: .fill 16
0D        ANS: .fill 0                    ! should come out to 65536
        
POW: 
0E  addi $14, $14, 2                      ! push 2 slots onto the stack
0F  sw $15, -1($14)                       ! save RA to stack
10  sw $1, -2($14)                       ! save arg 0 to stack
11  beq $zero, $2, RET1        0x0B      ! if the power is 0 return 1
12  beq $zero, $1, RET0        0x0D      ! if the base is 0 return 0
13  addi $2, $2, -1                     ! decrement the power
14  addi $3, $zero, POW        0x0E      ! load the address of POW
15  jalr $3, $15                         ! recursively call POW
16  add $1, $10, $zero                   ! store return value in arg 1
17  lw $2, -2($14)                       ! load the base into arg 0
18  addi $3, $zero, MULT       0x23      ! load the address of MULT
19  jalr $3, $15                         ! multiply arg 0 (base) and arg 1 (running product)
1A  lw $15, -1($14)                       ! load RA from the stack
1B  addi $14, $14, -2                     ! pop the RA and arg 0 off the stack
1C  jalr $15, $zero                       ! return
1D RET1: addi $10, $zero, 1               ! return a value of 1
1E  addi $14, $14, -2
1F  jalr $15, $zero
20 RET0: add $10, $zero, $zero            ! return a value of 0
21  addi $14, $14, -2
22  jalr $15, $zero                
        
23 MULT: add $10, $zero, $zero            ! zero out return value
24 AGAIN: add $10,$10, $1                ! multiply loop
25  nand $a2, $zero, $zero
26  add $2, $2, $a2
27  beq $2, $zero, DONE        0x01      ! finished multiplying
28  beq $zero, $zero, AGAIN     0xFB (-5) ! loop again
29 DONE: jalr $15, $zero        

2a initsp: .fill 0xA00000
