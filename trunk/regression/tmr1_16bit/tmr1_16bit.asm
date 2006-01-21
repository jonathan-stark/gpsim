        ;; tmr0_16bit.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; TMR0 for a 16bit-core pic (like the 18fxxx family)
        ;; Here are the tests performed:
	;;
	;; -- TMR0L and TMR0H can be read and written
	;; -- Writing to TMR0L 

	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros
        radix   dec                     ; Numbers are assumed to be decimal



;----------------------------------------------------------------------

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1

TMR0_RollOver	RES	1
TMR1_RollOver	RES	1
countLo		RES	1
countHi		RES	1
TMR1H_Sampled	RES	1
TMR1H_Last	RES	1
SecondsLo_2	RES	1
SecondsHi_2	RES	1

  GLOBAL countLo, countHi
  GLOBAL SecondsLo_2, SecondsHi_2

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


    ;;==================
    ;; TMR1 Interrupt
    ;;==================

	BTFSC	INTCON,T0IF		;If the roll over flag is not set
	 BTFSS	INTCON,T0IE		;or the interrupt is not enabled
	  bra   checkTMR0IntEnd         ;

    ;; TMR0 has rolled over. Clear the pending interrupt and notify
    ;; the foreground code by incrementing the "roll over" counter
    ;; Notice that the increment operation is atomic - this means
    ;; we won't have to worry about the foreground code and interrupt
    ;; code clashing over accesses to this variable.
	
        BCF     INTCON,T0IF,0           ; Clear the pending interrupt
        INCF    TMR0_RollOver,F         ; Set a flag to indicate rollover

checkTMR0IntEnd:

    ;;==================
    ;; TMR1 Interrupt
    ;;==================
	BTFSC	PIR1,TMR1IF
	 BTFSS	PIE1,TMR1IE
	  BRA	checkTMR1IntEnd

	BCF	PIR1,TMR1IF		;Clear the interrupt
	INCF	TMR1_RollOver,F		;Set a flag

   ; Note, since we have a 32768 Hz crystal tied to TMR1 and we have the timer
   ; configured as a 16-bit timer then this interrupt will happen once every
   ; two seconds.

	INFSNZ	SecondsLo_2,F		;Update the system time.
	 INCF	SecondsHi_2,F

	CLRF	TMR1H_Sampled
	CLRF	TMR1H_Last
checkTMR1IntEnd:

ExitInterrupt:
	RETFIE	1


MAIN	CODE

Start:	

  ;------------------------------------------------------------------------
  ;
  ; Embedded simulation scripts
  ;
  ; Set the clock frequency to 40MHz

  .sim "frequency 40e6"


  ; define a stimulus to generate a 32kHz clock. This will be tied
  ; to TMR1's input and is used to simulate a crystal.
  ;
  ; 40MHz 
  ;   Instruction Rate = (4 clks/instruction) / (40e6 clks/second)
  ;                    = 100nS per instruction
  ;  32.768kHz ==> 30.517uS period
  ;  Instruction Cycles/32.768kHz cycle = 30.517uS / .1uS 
  ;                                     = 305.175 instructions
  ;

  .sim "stimulus asynchronous_stimulus "
  .sim "initial_state 0"
  .sim "start_cycle 0"
  .sim "period 305"
  .sim "{ 152, 1}"
  .sim "name Clk32kHz"
  .sim "end"

  ; Create a node

  .sim "node clk_node"

  ; attach the clock

  .sim "attach clk_node  Clk32kHz portc0"



  ; At reset, all bits of t1Con should be clear.
  .assert "t1con == 0x00"

	MOVLW	(1<<RC0)|(1<<RC1)  ;The lower two bits of PORTC are for
	IORWF	TRISC,F		   ;the 32kHz oscillator and should be inputs

	MOVLW	(1<<RD16) | (1<<T1OSCEN) | (1<<TMR1CS) | (1<<TMR1ON)
	MOVWF	T1CON
	BCF	PIR1,TMR1IF	;Clear any TMR1 pending interrupt
	BSF	PIE1,TMR1IE	;Enable TMR1 interrupts
	BSF	INTCON,PEIE	;Enable Peripheral interrupts
	BSF	INTCON,GIE	;Enable Global interrupts

	clrf	countLo
	clrf	countHi


L1:	clrwdt
	rcall	d1
	decfsz	countLo,F
	 bra	L1
	decfsz	countHi,F
	 bra	L1

	nop
done:
  .assert  "\"*** PASSED 16bit-core TMR1 test\""
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "\"*** FAILED 16bit-core TMR1 test\""
        bra     done

d1	RCALL	d2
d2	RCALL	d3
d3	RCALL	d4
d4	RCALL	d5
d5	RCALL	d6
d6	RCALL	d7
d7	RCALL	d8
d8	RCALL	d9
d9	RCALL	d10
d10	RCALL	d11
d11	RETURN

 end
