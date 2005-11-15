
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


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
start
   .sim "module library libgpsim_modules"
   .sim "module load switch SW1"
   .sim "module load pullup R1"
   .sim "module load pullup R2"

   .sim "node nb0"
   .sim "node nc0"

   .sim "attach nb0 portb0 SW1.A R1.pin"
   .sim "attach nc0 portc0 SW1.B R2.pin"

   ; First make sure that the switch is open
   .sim "SW1.state=false"


;   .sim "set verbose 0xff"


	MOVLW	0xff
	BSF	STATUS,RP0

	CLRF	TRISB^0x80	;Port B is an output
	MOVWF	TRISC^0x80	;Port C is an input

	BCF 	STATUS,RP0

   ; The switch is open and portb0 is driving one side of it.
   ; The other side goes to portc0 and is pulled up by the pullup resistor
   ; Toggling portb0 should have no effect on portc0

	BCF	PORTB,0

   .assert "(portc & 1) == 1"

	nop

	BSF	PORTB,0

   .assert "(portc & 1) == 1"

	nop

   ; Close the switch:	
   .command "SW1.state=true"
	nop

   .assert "(portc & 1) == 1"
	nop

	BCF	PORTB,0

   .assert "(portc & 1) == 0"

	nop

done:

  .assert  ",\"*** PASSED Switch Test on 16f877\""
	goto	$

FAILED:
  .assert  ",\"*** FAILED Switch Test on 16f877\""
	goto	$



  end
