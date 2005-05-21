;************************************************************************
;
; Assertion test
;
;************************************************************************

	list      p=16f873                   ; list directive to define processor
	#include <p16f873.inc>               ; processor specific variable definitions
	__CONFIG (_CP_OFF & _WDT_ON & _BODEN_ON & _PWRTE_ON & _HS_OSC & _WRT_ENABLE_ON & _LVP_OFF & _CPD_OFF)

	errorlevel -302	

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA		UDATA
var1		RES	1
var2		RES	1
var3		RES	1

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

	clrf	var1
	clrf	var2
	clrf	var3

  .direct "A",  "var1==5"
	nop

  .direct "A",  "var2==6"
	nop
done:
  .direct "A",  ""
	goto	done


	nop
  end
