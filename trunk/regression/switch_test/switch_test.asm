
   ;;  Switch test 
   ;;
   ;;  Test switch with capacitance and without capacitance


	list    p=16f877                ; list directive to define processor
	include <p16f877.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros


;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1		RES	1
temp2		RES	1

  GLOBAL done



;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE


;----------------------------------------------------------------------
; gpsim configuration script
;
; The configuration environment consists of a switch connecting
; portb0 to portc1. Resistors on either side of the switch
; ensure that the nodes are never floating.
;
;               PU1
;                |   \
;  portb0 <>--+--+--O \O--+-- portc1
;  portc0  <--|           |
;                        PD1
;
;

   ;# Module libraries:
   .sim "module library libgpsim_modules"
   .sim "p16f877.xpos = 48.0"
   .sim "p16f877.ypos = 36.0"

   .sim "module load switch SW1"
   .sim "SW1.state=false"
   .sim "SW1.xpos = 216.0"
   .sim "SW1.ypos = 156.0"
   .sim "SW1.Rclosed = 100.0"
   .sim "SW1.Ropen = 1.0e8"

   .sim "module load pullup PU1"
   .sim "PU1.resistance=1.0e8"
   .sim "PU1.capacitance=1.00e-07"
   .sim "PU1.xpos = 204.0"
   .sim "PU1.ypos = 36.0"

   .sim "module load pulldown PD1"
   .sim "PD1.resistance=10000."
   .sim "PD1.capacitance=2.00e-06"
   .sim "PD1.xpos = 204.0"
   .sim "PD1.ypos = 96.0"

    ;# Connections:
   .sim "node nb0"
   .sim "attach nb0 SW1.A portb0 portc0 PU1"

   .sim "node nc0"
   .sim "attach nc0 SW1.B PD1.pin portc1"

   .sim "frequency 10.0e6"

;# End of configuration
;
;------------------------------------------------------------------------


start

	BSF	STATUS,RP0

	movlw	0x03		; RC1,RC0 are inputs
	movwf	TRISC
	CLRF	TRISB^0x80	;Port B is an output

	BCF 	STATUS,RP0

   ; The switch is open and portb0 is driving one side of it.
   ; The other side goes to portc0 and is pulled up by the pullup resistor
   ; Toggling portb0 should have no effect on portc0

	BCF	PORTB,0

   .assert "(portc & 3) == 0"	; both sides of switch should be 0

	nop

	BSF	PORTB,0

   .assert "(portc & 3) == 1"	; drive side only high

	nop

   ; Close the switch:	
   .command "SW1.state=true"
	nop

   .assert "(portc & 3) == 1"	; drive side only high because of capacitance
 	nop

       call    delay

   .assert "(portc & 3) == 3"	
	nop

	BCF	PORTB,0		; change drive voltage



       call    delay

   .assert "(portc & 3) == 0"   ; both sides now low
	nop
	movf	PORTC,W

	BSF	PORTB,0		; drive high again
	call	delay

   .assert "(portc & 3) == 3"	; make sure both sides are high
	nop

   ; Open the switch:	
   .command "SW1.state=false"
	nop

;   .assert "(portc & 3) == 3"	; capacitance should hold both sides high
;				; for a while
;	nop
;
;	call delay

   .assert "(portc & 3) == 1"   ; only one side high
	nop

	BCF	PORTB,0

   .assert "(portc & 3) == 0"
	nop

;	Turn off Capacitance to test DC behaviour
;
   .command "PD1.capacitance=0.0"
	nop
;   .command "PU1.capacitance=0"
	nop

   .assert "(portc & 3) == 0"
	nop

	BSF	PORTB,0		; Drive one side of switch high
   .assert "(portc & 3) == 1"
	nop

   ; Close the switch:	
   .command "SW1.state=true"
	nop

   .assert "(portc & 3) == 3"	; Both side should now be high
	nop

	BCF	PORTB,0
   .assert "(portc & 3) == 0"	; Both side should now be low
	nop
done:

  .assert  "\"*** PASSED Switch Test on 16f877\""
	goto	$

FAILED:
  .assert  "\"*** FAILED Switch Test on 16f877\""
	goto	$


delay		; delay about 11,500 cycles or  1.2 ms at 10 Mhz
       movlw   9
       movwf	temp2
       clrf    temp1
delay_loop
        decfsz  temp1,f
         goto   $+2
        decfsz  temp2,f
         goto   delay_loop
        return


  end
