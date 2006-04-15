	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1

a2dIntFlag	RES	1  ;LSB is set when an A2D interrupt occurs

  GLOBAL done
  GLOBAL a2dIntFlag

;------------------------------------------------------------------------
STARTUP    CODE	0

	bra	Start

;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location


check_TMR0_interrupt:

	btfsc	PIR1,ADIF	;If A2D int flag is not set
	 btfss	PIE1,ADIE	;Or the interrupt is not enabled
	  RETFIE 1		; Then leave

;;	An A/D interrupt has occurred

	bsf	a2dIntFlag,0	;Set a flag to indicate we got the int.
	bcf	ADCON0,ADIF	;Clear the a/d interrupt

ExitInterrupt:
	RETFIE	1


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE


   .sim "module library libgpsim_modules"
   ; Use a pullup resistor as a voltage source
   .sim "module load pullup V1"
   .sim "V1.resistance = 100.0"

   .sim "node na0"
   .sim "attach na0 V1.pin porta0"


Start:


    ; RA0 is an Analog Input.
    ; RA1 - RA5 are all configured as outputs.
    ;
    ; Use VDD and VSS for Voltage references.
    ;
    ; PCFG = 1110  == AN0 is the only analog input
    ; ADCS = 110   == FOSC/64
    ; ADFM = 0     == 6 LSB of ADRESL are 0.
    ;

	MOVLW	1<<RA0
	MOVWF	TRISA

	MOVLW	(1<<ADCS2) | (1<<PCFG1) | (1<<PCFG2) | (1<<PCFG3)
	MOVWF	ADCON1
	MOVLW	(1<<ADCS1) | (1<<ADON)
	MOVWF	ADCON0

	BSF	INTCON,GIE	;Global interrupts
	BSF	INTCON,PEIE	;Peripheral interrupts
	BSF	PIE1,ADIE	;A2D interrupts

	RCALL	Convert


done:
  .assert  "\"*** PASSED 18F452 a2d test\""
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "\"*** FAILED 18F452 a2d test\""
        bra     done


Convert:
	BCF	a2dIntFlag,0	;Clear interrupt handshake flag

	BSF	ADCON0,GO

LWait:	
	BTFSS	a2dIntFlag,0	;Wait for the interrupt to set the flag
	 bra	LWait

	MOVF	ADRESH,W		;Read the high 8-bits of the result

	RETURN

        end
