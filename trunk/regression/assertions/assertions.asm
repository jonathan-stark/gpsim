;************************************************************************
;
; Assertion test
;
;************************************************************************

	list      p=16f873              ; list directive to define processor
	include <p16f873.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG (_CP_OFF & _WDT_ON & _BODEN_ON & _PWRTE_ON & _HS_OSC & _WRT_ENABLE_ON & _LVP_OFF & _CPD_OFF)

        errorlevel -302 

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
var1            RES     1
var2            RES     1
var3            RES     1

  GLOBAL var1,var2,var3

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

        clrf    var1
        clrf    var2
        clrf    var3

  ; var1 and var2 have just been cleared, so the following two
  ; assertions should fail. However the script controlling the
  ; this regression test expects this and will continue running

  .assert  "var1==5"
        nop
  .assert  "var2==6"
        nop

  ; compound expression
  .assert  "var2!=0 || var1!=0"
        nop

done:
  ; If no expression is specified, then break unconditionally
  .assert  ""
        goto    done


        nop
  end
