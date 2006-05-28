
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
; The configuration environment consists of a switch SW1 connecting
; portb0 and portc0 to portc1. Resistors on either side of the switch
; ensure that the nodes are never floating and allow the switch voltage
; to be checked.
;
; SW2.A is connected to the same node as SW1.A and SW2.B is connected to
; portc2. This allows checking of 2 switch infinite regression bug.
;
; Portb0 is used to drive the switch while portc 0-2 monitor the results
;
;               PU1
;                |   \
;  portb0  >--+--+--O \O--+--> portc1
;  portc0  <--|           |
;             |      \   PD1
;             |-----O \O-----> portc2
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

   .sim "module load switch SW2"
   .sim "SW2.state=false"
   .sim "SW2.xpos = 216.0"
   .sim "SW2.ypos = 228.0"
   .sim "SW2.Rclosed = 100.0"
   .sim "SW2.Ropen = 1.0e6"

    ;# Connections:
   .sim "node nb0"
   .sim "attach nb0 SW1.A portb0 portc0 PU1.pin SW2.A"

   .sim "node nc0"
   .sim "attach nc0 SW1.B PD1.pin portc1"

   .sim "node nc1"
   .sim "attach nc1 SW2.B portc2"

   .sim "frequency 10.0e6"

;# End of configuration
;
;------------------------------------------------------------------------


start

	BSF	STATUS,RP0

	movlw	0x07		; RC2,RC1 and RC0 are inputs
	movwf	TRISC
	CLRF	TRISB^0x80	;Port B is an output

	BCF 	STATUS,RP0

   ; The switch is open and portb0 is driving one side of it and portc0.
   ; The other side goes to portc1 and is pulled up by the pullup resistor
   ; Toggling portb0 should have no effect on portc1

	BCF	PORTB,0

   .assert "(portc & 3) == 0"	; both sides of switch should be 0

	nop

	BSF	PORTB,0

   .assert "(portc & 3) == 1"	; drive side only high

	nop

   ; Close the switch because of capacitance portc1 will go high after a delay:	
   .command "SW1.state=true"
	nop

   .assert "(portc & 3) == 1"	; drive side only high because of capacitance
 	nop

      call    delay

   .assert "(portc & 3) == 3"	
	nop

;  drive portb low, portc1 should be low after a delay
;
	BCF	PORTB,0		; change drive voltage



       call    delay

   .assert "(portc & 3) == 0"   ; both sides now low
	nop
	movf	PORTC,W

;
;	test capacitance delay on drive transition
;
	BSF	PORTB,0		; drive high again
	call	delay

   .assert "(portc & 3) == 3"	; make sure both sides are high
	nop

   ; Open the switch, because of small capacitance, floating side low
   ; immediately
   ;
   .command "SW1.state=false"
	nop

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

   ; Close the SW2 switch, portc2 should be same as portc0 and portc1

	BSF	PORTB,0		; Drive one side of switch high

   .command "SW2.state=true"
	nop

   .assert "(portc & 7) == 7"	; All inputs should now be high
	nop

	BCF	PORTB,0
   .assert "(portc & 7) == 0"	; All inputs should now be low
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
