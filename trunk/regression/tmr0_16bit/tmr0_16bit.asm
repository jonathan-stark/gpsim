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

  GLOBAL done

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

	CLRF	TMR0L
	MOVLW	(1<<TMR0ON) | (1<<T08BIT) | (1<<PSA)
	MOVWF	T0CON
  ;
  ; tmr0 is running now. It should be incrementing once every instruction cycle. 
	NOP
	NOP
	NOP
	NOP
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


        end
