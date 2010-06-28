
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


	list    p=16f628                ; list directive to define processor
	include <p16f628.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

;         __CONFIG _CP_OFF & _WDT_OFF &  _HS_OSC & _PWRTE_ON & _LVP_OFF & _BODEN_OFF & _MCLRE_ON
         ;;__CONFIG _CP_OFF & _WDT_OFF &  _ER_OSC_CLKOUT & _PWRTE_ON & _LVP_OFF & _BODEN_OFF & _MCLRE_ON
;         __CONFIG _CP_OFF & _WDT_OFF &  _INTRC_OSC_NOCLKOUT & _PWRTE_ON & _LVP_OFF & _BODEN_OFF & _MCLRE_ON
        __CONFIG _CP_OFF & _WDT_OFF &  _INTRC_OSC_NOCLKOUT & _PWRTE_ON & _LVP_OFF & _BODEN_OFF & _MCLRE_OFF


;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
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

       btfsc   PIR1,CMIF 
         btfss  PIE1,CMIE
	goto exit_int

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
	movlw	0x07
	movwf	CMCON		; turn off Comparator
;
; test bits 5, 6 and 7 work
;
	clrf	PORTB
	bsf	STATUS,RP0
	clrf	TRISB^0x80	;Port B is an output
	bcf	STATUS,RP0
  .assert "porta == 0, \"Porta = 0\""
	nop
	movlw	0xff
	movwf	PORTB		; drive all bits high
  .assert "porta == 0xff, \"Porta = 0xff\""
	nop
	clrf	PORTA
	bsf     STATUS,RP0
        clrf    TRISA^0x80      ;Port A is an output
	movlw	0xff
	movwf	TRISB^0x80	;Port B is an input
        bcf     STATUS,RP0
  .assert "portb == 0, \"Portb = 0\""
	nop
	movlw	0xff
	movwf	PORTA		; drive all bits high
  .assert "portb == 0xff, \"Portb = 0xff\""
	nop

;
;	test mode 2 - Four Inputs muxed to two comparators with Vref
;

        movlw	0x02		; comparater mux reference
        movwf	CMCON
	bcf	PIR1,CMIF
	clrf	PORTA
	bsf	STATUS,RP0

	movlw	0x0f		; RA3-RA0 are inputs
	movwf	TRISA
	movlw	0xAB		; enable Vref 11 low range
	movwf	VRCON
  .assert "(cmcon & 0x40) == 0x00, \"FAILED 16f628 m2 C1OUT=0 \""
	nop
	movlw	0x8B		; enable Vref 11 high range
	movwf	VRCON
  .assert "(cmcon & 0x40) == 0x40, \"FAILED 16f628 m2 C1OUT=1 \""
	nop
	clrf	TRISB^0x80	;Port B is an output
	bcf 	STATUS,RP0

	clrf	PORTB
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m2 C1OUT=1 C2OUT=1 \""
	nop
  .assert "(pir1 & 0x40) == 0x40, \"FAILED 16f628 m2 CMIF=1\""
	nop
	bcf	PIR1,CMIF
  .assert "(pir1 & 0x40) == 0x00, \"FAILED 16f628 m2 CMIF=0\""
	nop
	bsf	PORTB,0			; drive comp 1 input high
  .assert "(cmcon & 0xc0) == 0x80, \"FAILED 16f628 m2 C1OUT=0 C2OUT=1 \""
        nop
  .assert "(pir1 & 0x40) == 0x40, \"FAILED 16f628 m2 CMIF=1 on input change\""
	nop
	bcf	PIR1,CMIF
        movlw	0x32		; comparater mux reference invert output
        movwf	CMCON
        movf    CMCON,W                 ; This updates compare value
  .assert "(cmcon & 0xc0) == 0x40, \"FAILED 16f628 m2 C1OUT=1 C2OUT=0 C1INV=C2INV=1\""
	nop
  .assert "(pir1 & 0x40) == 0x40, \"FAILED 16f628 m2 CMIF=1 on invert change\""
	nop

	bcf	PIR1,CMIF
	bsf	PORTB,1			; drive comp 2 input high
	bcf	PORTB,0			; drive comp 1 input low
  .assert "(cmcon & 0xc0) == 0x80, \"FAILED 16f628 m2 C1OUT=0 C2OUT=1 C1INV=C2INV=1\""
	nop
  .assert "(pir1 & 0x40) == 0x40, \"FAILED 16f628 m2 CMIF=1 input change with invert\""
	nop
	bcf	PIR1,CMIF

        movlw	0x02		; comparater mux reference normal output
        movwf	CMCON
        movf    CMCON,W                 ; This updates compare value
  .assert "(cmcon & 0xc0) == 0x40, \"FAILED 16f628 m2 C1OUT=1 C2OUT=0 INV off\""
	nop
  .assert "(pir1 & 0x40) == 0x40, \"FAILED 16f628 m2 CMIF=1 invert off\""
	nop
	bcf	PIR1,CMIF
	bsf	STATUS,RP0
	bsf	PIE1,CMIE	; enable Comparator interrupts
	bcf	STATUS,RP0
	bsf	INTCON,PEIE	; enable Periperal inturrupts
	bsf	INTCON,GIE	; and globale interupts
	bsf	PORTB,0		; drive comp1 input high
	nop
        nop
	goto FAILED		; no interrupt
done1:
;
;	mode 1 - Three Inputs Muxed to Two Comparators
;
	movlw	0x04
	movwf	PORTB
        movlw	0x01
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m1 C1OUT=1 C2OUT=1 \""
	nop
        movlw	0x09			; Select pin 3 Comp 1
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m1 C1OUT=1 C2OUT=1 using pin 3\""
	nop
	movlw	0x03
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x00, \"FAILED 16f628 m1 C1OUT=0 C2OUT=0 \""
	nop
        movlw	0x01			; Select pin 0 Comp 1
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0x00, \"FAILED 16f628 m1 C1OUT=0 C2OUT=0 select pin 0\""
	nop

;
;	mode 3 - Two Common reference Comparators
;
	movlw	0x04
	movwf	PORTB
        movlw	0x03
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m3 C1OUT=1 C2OUT=1 \""
	nop
	movlw	0x03
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x00, \"FAILED 16f628 m3 C1OUT=0 C2OUT=0 \""
	nop
	movlw	0x0c
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m3 C1OUT=1 C2OUT=1 bit 3 no effect\""
	nop

;
;	mode 4 - Two independant Comparators
;
	movlw	0x0a
	movwf	PORTB
        movlw	0x04
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0x40, \"FAILED 16f628 m4 C1OUT=1 C2OUT=0 \""
	nop
	movlw	0x05
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x80, \"FAILED 16f628 m4 C1OUT=0 C2OUT=1 \""
	nop
	movlw	0x0c
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m4 C1OUT=1 C2OUT=1 \""
	nop

;
;	mode 5 - One independant Comparator
;
	movlw	0x0a
	movwf	PORTB
        movlw	0x05
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0x00, \"FAILED 16f628 m5 C1OUT=0 C2OUT=0 \""
	nop
	movlw	0x05
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x80, \"FAILED 16f628 m5 C1OUT=0 C2OUT=1 \""
	nop
	movlw	0x0c
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x80, \"FAILED 16f628 m5 C1OUT=0 C2OUT=1 pins 1,3 don't care\""
	nop
        movlw	0x15			; Invert Comp 1 should have no effect
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0x80, \"FAILED 16f628 m5 C1OUT=0 C2OUT=1 C1INV=1\""
	nop

;
;	mode 6 - Two Common reference Comparators with output 
;
	bsf	STATUS,RP0
	movlw	0x07		; RA2-RA0 are inputs
	movwf	TRISA
	movlw	0x18		; RB3, RB4 are inputs
	movwf	TRISB
	bcf	STATUS,RP0

	movlw	0x04
	movwf	PORTB
        movlw	0x06
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m6 C1OUT=1 C2OUT=1 \""
	nop
  .assert "(portb & 0x18) == 0x18, \"FAILED 16f628 m6 portb3=1 portb4=1\""
	nop
	movlw	0x03
	movwf	PORTB
  .assert "(cmcon & 0xc0) == 0x00, \"FAILED 16f628 m6 C1OUT=0 C2OUT=0 \""
	nop
  .assert "(portb & 0x18) == 0x00, \"FAILED 16f628 m6 portb3=0 portb4=0\""
	nop
        movlw	0x16			; invert COMP1 output
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0x40, \"FAILED 16f628 m6 C1OUT=1 C2OUT=0 C1INV=1\""
	nop
  .assert "(portb & 0x18) == 0x08, \"FAILED 16f628 m6 portb3=1 portb4=0\""
	nop
        movlw	0x36			; invert COMP1  & COMP2 output
        movwf	CMCON
  .assert "(cmcon & 0xc0) == 0xc0, \"FAILED 16f628 m6 C1OUT=1 C2OUT=1 C1INV=1 C2INV=1\""
	nop
  .assert "(portb & 0x18) == 0x18, \"FAILED 16f628 m6 portb3=1 portb4=1 C1INV=1 C2INV=1\""
	nop

  .assert  "\"*** PASSED 16f628 Comparator\""
	goto	$

FAILED:
  .assert  "\"*** FAILED 16f628 No comparator interrupt\""
	goto	$



  end
