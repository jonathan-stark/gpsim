        radix dec
        ;; The purpose of this regression test is to test
        ;; digital stimuli.

	list    p=16f84                 ; list directive to define processor
	include <p16f84.inc>            ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1


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
        clrf    failures
        clrf    temp1           ; a counter
        clrf    temp2           ; a flag keeping track of the state of port b
begin:

        ;; Count the rising edges on portb bit 0
        
        movf    PORTB,W
        xorwf   temp2,w
        xorwf   temp2,f
        andwf   temp2,w
        andlw   1
        
        skpz
          incf  temp1,f
        
        btfss   PORTB,1
         goto   begin

        movf    temp1,W

#define EXPECTED_PULSES .20

        xorlw   EXPECTED_PULSES
        skpz
  .assert  "\"*** FAILED digital stimulus test\""
         incf   failures,F
done:
  .assert  "\"*** PASSED digital stimulus test\""
        goto    done

        end
