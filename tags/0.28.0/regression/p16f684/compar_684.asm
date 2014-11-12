
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


	list    p=16f684               ; list directive to define processor
	include <p16f684.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG  _EXTRC_OSC_NOCLKOUT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm

CMODE0                         EQU     H'0000'
CMODE1                         EQU     H'0001'
CMODE2                         EQU     H'0002'
CMODE3                         EQU     H'0003'
CMODE4                         EQU     H'0004'
CMODE5                         EQU     H'0005'
CMODE6                         EQU     H'0006'
CMODE7                         EQU     H'0007'


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
temp            RES     1

w_temp          RES     1
status_temp     RES     1

  GLOBAL done1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

       btfsc   PIR1,C1IF 
	 goto icm1
	goto exit_int

icm1:
	bcf PIR1,C1IF

	goto done1

exit_int:
                                                                                
        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
  .assert "cmcon0 == 0x00, \"FAILED 16f684 CMCON0 POR\""	; Rest value
	nop
	BANKSEL	PIR1
	clrf	PIR1
        movlw	CMODE7		; turn off comparater 
        movwf	CMCON0
	BANKSEL ANSEL
	clrf	ANSEL
	movlw	0x0f		; RA3-RA0 are inputs
	movwf	TRISA
	movlw	0xAB		; enable Vref 11 low range
	movwf	VRCON
;
;	test mode 2 - Four Inputs muxed to two comparators with Vref
;
        movlw	CMODE2		; comparater mux reference
	BANKSEL CMCON0
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x80, \"FAILED 16f684 m2 C1OUT=0 C2OUT=1\""
	nop
  .assert "(pir1 & 0x18) == 0x10, \"FAILED 16f684 C1if=0 C2IF=1\""
	nop
	clrf	PIR1
	movlw	0x8B		; enable Vref 11 high range
        BANKSEL VRCON
	movwf	VRCON
  .assert "(cmcon0 & 0xc0) == 0xc0, \"FAILED 16f684 m2 C1OUT=1 C2OUT=1\"" 
	nop
	bcf	OPTION_REG,T0CS		; Timer not from RA4
	movlw   0x03
	movwf	TRISC

	BCF 	STATUS,RP0
  .assert "(cmcon0 & 0xc0) == 0xc0, \"FAILED 16f684 m2 C1OUT=1 C2OUT=1\""
	nop
  .assert "(pir1 & 0x18) == 0x08, \"FAILED 16f684 C1IF=1 C2IF=0\""
	nop
	bcf	PIR1,C1IF
  .assert "(pir1 & 0x18) == 0x00, \"FAILED 16f684 C1IF=0 C2IF=0\""
	nop
	bsf	PORTC,5			; drive comp 1 input high
  .assert "(cmcon0 & 0xc0) == 0x80, \"FAILED 16f684 m2 C1OUT=0 C2OUT=1\""
        nop
  .assert "(pir1 & 0x18) == 0x08, \"FAILED 16f684 C1IF=1\""
	nop
	bcf	PIR1,C1IF
	bsf	CMCON0,C1INV		; invert output
	bsf	CMCON0,C2INV
  .assert "(cmcon0 & 0xc0) == 0x40, \"FAILED 16f684 m2 invert C1OUT=1 C2OUT=0\""
	nop
  .assert "(pir1 & 0x18) == 0x18, \"FAILED 16f684 C1IF=1\""
	nop

	bcf	PIR1,C1IF
	bcf	PIR1,C2IF
	bsf	PORTA,4			; drive comp 2 input high
	bcf	PORTC,5			; drive comp 1 input low
	bsf	PORTC,3			; drive comp 1 input 2 high
  .assert "(cmcon0 & 0xc0) == 0x80, \"FAILED 16f684 m2 new inputs C1OUT=0 C2OUT=1\""
	nop
  .assert "(pir1 & 0x18) == 0x18, \"FAILED 16f684 C1IF=1 C2IF=1\""
	nop
	bsf	CMCON0,CIS	; switch pins
  .assert "(cmcon0 & 0xc0) == 0x40, \"FAILED 16f684 m2 new inputs C1OUT=1 C2OUT=0\""
	nop

        movlw	CMODE2		; comparater mux reference
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x40, \"FAILED 16f684 m6 invert normal C1OUT=1 C2OUT=0\""
	nop
	clrf	PIR1
	BANKSEL PIE1
	bsf	PIE1,C1IE	; enable Comparator interupts
	bsf	PIE1,C2IE	; enable Comparator interupts
	bsf	INTCON,PEIE	; enable Periperal inturrupts
	bsf	INTCON,GIE	; and globale interupts
	BANKSEL PORTC
	bsf	PORTC,5		; drive comp1 input low to trigger interrupt
	nop
        nop
	goto FAILED		; no interrupt
done1:
;
;	mode1 - 3 inputs mux to two comparators
;
	bsf	PORTC,3 	; c1- = H
	bcf	PORTC,5		; c1+ = 0
	bcf	PORTC,2		; c2- = 0
	bsf	PORTA,4		; cx+ = H

        movlw	CMODE1
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x80, \"FAILED 16f684 m1 C1OUT=0 C2OUT=1\""
	nop
	bsf	CMCON0,CIS
  .assert "(cmcon0 & 0xc0) == 0xc0, \"FAILED 16f684 m1 C1OUT=1 C2OUT=1\""
	nop
;
;	mode 4 - Two independant Comparators
;
;		Both Comparator true
	bcf	PORTC,3 	; c1- = 0
	bsf	PORTC,5		; c1+ = H
	bcf	PORTC,2		; c2- = 0
	bsf	PORTA,4		; c2+ = H
        movlw	CMODE4
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0xc0, \"FAILED 16f684 m4 C1OUT=1 C2OUT=1\""
	nop
;		C1 high only
	bcf     PORTA,4
	bsf     PORTC,2
  .assert "(cmcon0 & 0xc0) == 0x40, \"FAILED 16f684 m4 C1OUT=1 C2OUT=0\""
	nop
;		C2 only high
	bsf	PORTC,3 	; c1- = H
	bcf	PORTC,5		; c1+ = 0
	bcf	PORTC,2		; c2- = 0
	bsf	PORTA,4		; c2+ = H
  .assert "(cmcon0 & 0xc0) == 0x80, \"FAILED 16f684 m4 C1OUT=0 C2OUT=1\""
	nop
;		both low
	bsf	PORTC,2		; c2- = H
	bcf	PORTA,4		; c2+ = 0
  .assert "(cmcon0 & 0xc0) == 0x00, \"FAILED 16f684 m4  C1OUT=0 C2OUT=0\""
	nop

;
;	mode 3 - Two common reference Comparators
;
        movlw	CMODE3
        movwf	CMCON0
	bsf	PORTC,3 	; c1- = H
	bcf	PORTC,2		; c2- = 0
	bsf	PORTA,4		; cx+ = H
  .assert "(cmcon0 & 0xc0) == 0x80, \"FAILED 16f684 m3 C1OUT=0 C2OUT=1\""
	nop
	bcf	PORTC,3 	; c1- = 0
	bsf	PORTC,2		; c2- = H
  .assert "(cmcon0 & 0xc0) == 0x40, \"FAILED 16f684 m3 C1OUT=1 C2OUT=0\""
	nop
	bcf	PORTC,2		; c2- = 0
  .assert "(cmcon0 & 0xc0) == 0xc0, \"FAILED 16f684 m3 C1OUT=1 C2OUT=1\""
	nop

;
;	mode 5 - One independant Comparator 
;
        movlw	CMODE5
        movwf	CMCON0
  .assert "(cmcon0 & 0xc0) == 0x80, \"FAILED 16f684 m5 C1OUT=0 C2OUT=1\""
	nop

;
;	mode 6 two common reference Comparators with Outputs
;
	BANKSEL TRISA
	bcf	TRISA,2
	BANKSEL CMCON0
        movlw	CMODE6
        movwf	CMCON0

  .assert "(cmcon0 & 0xc0) == 0xc0, \"FAILED 16f684 m6 C1OUT=1 C2OUT=1\""
	nop
  .assert "(porta & 0x04) == 0x04,  \"FAILED 16f684 m6 porta2=1\""
	nop
  .assert "(portc & 0x10) == 0x10,  \"FAILED 16f684 m6 portc4=1\""
	nop

	bcf	PORTA,4		; cx+ = 0
  .assert "(cmcon0 & 0xc0) == 0x00, \"FAILED 16f684 m6 C1OUT=0 C2OUT=0\""
	nop
  .assert "(porta & 0x04) == 0x00,  \"FAILED 16f684 m6 porta2=0\""
	nop
  .assert "(portc & 0x10) == 0x00,  \"FAILED 16f684 m6 portc4=0\""
	nop

  .assert  "\"*** PASSED Comparator on 16f684\""
	goto	$

FAILED:
  .assert  "\"*** FAILED Comparator no interupt 16f684\""
	goto	$



  end
