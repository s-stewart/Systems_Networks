!$zero should be translated to the register which is hardwired to 0
!$sp should be translated to your stack pointer register
!$r# should be translated to your choice of non specific use registers 
!	such that the ones with the same name are the same register
!	and those witth different names are different
!.fill xxx means to put xxx at that address
!name: meant this line is labeled 'name'
!using name in an offset later means the address of that line
! the number at the beginning of each line is the address of that line in hex
! for your convenience 

main:
0	addi $29, $zero, initsp ! initialize the stack pointer
4	lw $29, 0($29)
8	ei 			!Don't forget to enable interrupts...

		
C	addi $r1, $zero, 100	!load itteration count
10	addi $r4, $zero, loop
14 loop:			!this and the next line are the same address since labels dont take up a spot
14	lw $r3, $r1, 0		!should be an unaligned access 3/4 of the time
18	addi $r1, $r1, -1
1C	beq $zero, $r1, +4
20	jalr $r4, $zero		!remember this means the target is r4
	!beq target	
24	halt			!shold have caused 75 errors branch to self beq R0.R0,-4
	

28 initsp: .fill 0xA000
		
		
2C bad_access: .fill 0
30 mem_inthandler:
	!FIX ME
	!should record a misaligned access with maximum interruptable time
	!that is do all the overhead
	!and then increment address 2C
	! Fun Tip: If your offsets to sw/lw are atleast 9 bits you can say
	!	   sw/lw $r#, $zero, 2C 
	!	   (access at an offset of 2C from 0 will come out to 2C)
	!	   to save instructions which means less work for you
	
	di
	addi $29,$29,-4		!move stack pointer up
	sw $30,0($29)		!save PC value on the stack
	ei
	addi $29,$29,-4		!move stack pointer up
	sw $1,0($29)		!save register 1 on the stack for misalligned count use
	lw $1,2C($zero)		!load the interupt count into R1
	addi $1,$1,1  		!increment interupt count
	sw $1,44($0)		!store the increment count back to mem 2C = 44
	lw $30, 0($29)		!restore R1
	di
	addi $29,$29,4		!move stack pointer down
	lw $1,0($29)		!restore PC
	ei
	addi $29,$29,4		!move the stack pointer down
	RTI
	
	
	
	
