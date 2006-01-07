
   ;;  Resistor test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; resistor modules function.
   ;;
   ;;
   ;;


	list    p=16f873                ; list directive to define processor
	include <p16f873.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG (_CP_OFF & _WDT_ON & _BODEN_ON & _PWRTE_ON & _HS_OSC & _WRT_ENABLE_ON & _LVP_OFF & _CPD_OFF)

        errorlevel -302 

;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector

        movlw  high  start              ; load upper byte of 'start' label
        movwf  PCLATH                   ; initialize PCLATH
        goto   start                    ; go to beginning of program


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

   .sim "module library libgpsim_modules"
   .sim "module load pu pu1"
   .sim "module load pd pd1"
   .sim "pd1.resistance=3900."
   .sim "node A"
   .sim "attach A porta0 porta1 pu1.pin"
   .sim "node B"
   .sim "attach B porta2 portb0 pd1.pin"
;
;	the following test for a core dump bug
;
   .sim "node C"
   .sim "attach C pd1"


        nop
;
;	pull-up should set input ports high
    .assert "(porta&1) == 1"
;

        bsf     STATUS,RP0
	bcf	OPTION_REG,NOT_RBPU	; turn on portb pullups
;
;	 the pull-down resistor should pull down the weak B port pull-up
    .assert "(portb&1) == 0"
;
        movlw   0x81		;Port A 0 in others out
	movwf	TRISA
	bcf	OPTION_REG,NOT_RBPU	; turn on portb pullups


  ; Turn off the A2D converter and make PORTA's I/O's digital:

	movlw	(1<<PCFG1) | (1<<PCFG2)
	movwf	ADCON1

        bcf     STATUS,RP0

;
;	low output porta1 should drive pull-up resistor low
    .assert "(porta&1) == 0"
;

;
;	low output porta2 should drive portb0 and pull-down resistor low
    .assert "(portb&1) == 0"
;

	bsf	PORTA,1
;
;	high output porta1 high should drive porta0 and pull-up high
    .assert "(porta&1) == 1"
;
	bsf	PORTA,2
;
;	high output porta2 should drive portb0 and pull-down resistor high
    .assert "(portb&1) == 1"
;



passed:

done:
  ; If no expression is specified, then break unconditionally
                                                                                
#define UNRELEASED_GPASM
                                                                                
 ifdef UNRELEASED_GPASM
  .assert  ",\"*** PASSED Resistor Module test\""
 else
  .assert  ""
 endif
        goto    done
                                                                                
                                                                                
        nop
  end

end
