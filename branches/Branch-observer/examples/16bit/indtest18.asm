	;; indtest18.asm
	;;
	;; The purpose of this program is to test how well gpsim can simulate
	;; a 16bit-core pic (like the 18cxxx family not the 17c family).
	;; Nothing useful is performed - this program is only used to
	;; debug gpsim.
	;; indtest.asm tests indirect addressing. See it18.asm
	;; for a program that tests the instructions

	
include "p18c242.inc"

fast	equ	1
	
  cblock  0

	temp,temp1,temp2
  endc
	
	org 0


	movlb	0		;Point to bank 0


	lfsr	0,temp		;Point FSR 0 at the address of temp

	movlw	0x55
	movwf	indf0,0		;Write 0x55 to address 0
	xorwf	temp
	skpz
	 bra	$

	;; Fill a chunk of ram starting at temp2 with descending
	;; values

	lfsr	0,temp2
	movlw	0x10
	movwf	temp

l1:
	movff	temp,preinc0
	decfsz	temp,f
	 bra	l1

	;; Now access the same block of memory using post incrementing
	;; (The result is to zero out the whole block).

	lfsr	0,temp2+1
	movlw	0x10
	movwf	temp

l2:
	movf	temp,w
	subwf	postinc0,f,0
	skpz
	 bra	$
	
	decfsz	temp,f
	 bra	l2


	;; Fill a chunk of ram starting at temp2 with ascending
	;; values (using post-decementing)

	lfsr	0,temp2+0x10
	movlw	0x10
	movwf	temp

l3:
	movff	temp,postdec0
	decfsz	temp,f
	 bra	l3

	;; Now clear the block using plusw indirection
	lfsr	0,temp2
	movlw	0x10

l4:
	subwf	plusw0,f,0
	skpz
	 bra	$
	decfsz	wreg,f,0
	 bra	l4
	

	;; test indirect accesses of indirect registers
	lfsr	0,indf0
	call	indf_access

	lfsr	0,preinc0
	call	indf_access

	lfsr	0,postinc0
	call	indf_access

	lfsr	0,postdec0
	call	indf_access

	lfsr	0,plusw0
	call	indf_access

	movlw	0x55
	movwf	temp2
	lfsr	0,temp2
	movff	postdec0,preinc0
	
	bra	$

indf_access:
	movlw	0x55
	movwf	temp
	
	movff	indf0,temp	;Should write 0 to temp
	tstfsz	temp
	 bra	$

	movff	wreg,indf0	;This write should fail

	return
		
	end