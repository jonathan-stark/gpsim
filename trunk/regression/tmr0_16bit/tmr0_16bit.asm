        ;; tmr0_16bit.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; TMR0 for a 16bit-core pic (like the 18fxxx family)

	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros
        radix   dec                     ; Numbers are assumed to be decimal

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1

TMR0_RollOver	RES	1
tmr0Lo		RES	1
tmr0Hi		RES	1
psa		RES	1

  GLOBAL tmr0Lo, tmr0Hi
  GLOBAL psa
  GLOBAL done


DELAY	macro	delay
 local Ldc1, Ldc2

	MOVLW	delay
Ldc1:	ADDLW	-3	;3-cycle delay loop
	BC	Ldc1

  ; W now contains either -3 (0xFD), -2 (0xFE) or -1 (0xFF).
  ; The -2 case needs to be delayed an extra cycle more than
  ; the -3 case, and the -1 case needs yet another cycle of delay.
  ;
  ; Examine the bottom two bits and W to determine the exact delay
  ;

	BTFSS   WREG,1
	 BRA	Ldc2	;W=0xFD - no extra delay needed
	RRCF	WREG,F
	BC	Ldc2
Ldc2:
  endm

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
STARTUP    CODE	0

	bra	Start

;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location


check_TMR0_interrupt:

	BTFSC	INTCON,T0IF		;If the roll over flag is not set
	 BTFSS	INTCON,T0IE		;or the interrupt is not enabled
	  RETFIE 1			; Then leave

    ;; TMR0 has rolled over. Clear the pending interrupt and notify
    ;; the foreground code by incrementing the "roll over" counter
    ;; Notice that the increment operation is atomic - this means
    ;; we won't have to worry about the foreground code and interrupt
    ;; code clashing over accesses to this variable.
	
	BCF	INTCON,T0IF,0		; Clear the pending interrupt
	INCF	TMR0_RollOver,F		; Set a flag to indicate rollover

ExitInterrupt:
	RETFIE	1


MAIN	CODE

Start:	
  ; At reset, all bits of t0Con should be set.
  .assert "t0con == 0xff"
	nop


TMR0_8BitModeTest:
  ; Stop the timer and clear its value
	CLRF	T0CON
	CLRF	TMR0L
	CLRF	TMR0H
  .assert "(tmr0l == 0) && (tmr0h == 0)"

  ; The timer is off, so any value written to it should be read back
  ; unchanged.
	MOVLW	42
	MOVWF	TMR0L
  .assert "tmr0l == 42"

	MOVWF	TMR0H
  .assert "tmr0h == 42"


  ; 8-bit mode tests

	CLRF	psa
;	incf	psa,F
; 	bsf	psa,2

L1_tmr0_8BitModeTest:

	CLRF	T0CON
	CLRF	TMR0L
	CLRF	tmr0Lo

	MOVLW	(1<<TMR0ON) | (1<<T08BIT) | (1<<PSA)


	MOVF	psa,F
	BZ	L_psaInitComplete
	DECF	psa,W
	ANDLW	7
	IORLW	(1<<TMR0ON) | (1<<T08BIT)
		
L_psaInitComplete:
	MOVWF	T0CON
  ;
  ; tmr0 is running now. It should be incrementing once every instruction cycle. 
  ;
	MOVF	TMR0L,W
	MOVF	TMR0H,W
	MOVWF	tmr0Hi

  .assert "tmr0l == ((tmr0Lo+4)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+5)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+6)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+7)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+8)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+9)>>psa)"
	NOP
  .assert "tmr0l == ((tmr0Lo+10)>>psa)"
	NOP

  ; 8 23-cycle delays.

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+33)>>psa )"
	NOP

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+56)>>psa )"
	NOP

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+79)>>psa )"
	NOP

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+102)>>psa )"
	NOP

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+125)>>psa )"
	NOP

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+148)>>psa )"
	NOP

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+171)>>psa )"
	NOP

	MOVLW	23 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+194)>>psa )"
	NOP

  ; 255 and 256 cycle boundary condition

	MOVLW	61 - 11
	RCALL	DelayCycles
  .assert "tmr0l == ((tmr0Lo+255)>>psa )"
	NOP

  .assert "tmr0l == (((tmr0Lo+256)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+507)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+758)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+1009)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+1260)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+1511)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+1762)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+2013)>>psa)&0xff)"
	NOP

	MOVLW	251 - 11
	RCALL	DelayCycles
  .assert "tmr0l == (((tmr0Lo+2264)>>psa)&0xff)"
	NOP

	INCF	psa,F
	btfss	psa,3
	 bra	L1_tmr0_8BitModeTest

	NOP
	NOP
	NOP
	NOP

	BSF	INTCON,T0IE	;Enable TMR0 overflow interrupts



done:
  .assert  ",\"*** PASSED 16bit-core TMR0 test\""
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  ",\"*** FAILED 16bit-core TMR0 test\""
        bra     done

;------------------------------------------------------------
; DelayCycles:
;
; Input:   W -- the desired delay
; Output:  Returns after W+7 cycles

DelayCycles:

dc1:	ADDLW	-3	;3-cycle delay loop
	BC	dc1

  ; W now contains either -3 (0xFD), -2 (0xFE) or -1 (0xFF).
  ; The -2 case needs to be delayed an extra cycle more than
  ; the -3 case, and the -1 case needs yet another cycle of delay.
  ;
  ; Examine the bottom two bits and W to determine the exact delay
  ;

	BTFSS   WREG,1
	 BRA	dc2	;W=0xFD - no extra delay needed
	RRCF	WREG,F
	BC	dc2
dc2:    RETURN


 end
