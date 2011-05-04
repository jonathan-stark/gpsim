        ;; it.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; a 16bit-core pic (like the 18cxxx family not the 17c family.
        ;; Nothing useful is performed - this program is only used to
        ;; debug gpsim.

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


  GLOBAL done

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE

        clrf    temp1           ;Assume clrf works...
                                ;
        bra     start

failed1:        ; a relatively local label
        bra     failed

start:  
        ;; Perform some basic tests on some important instructions
done:
  .assert  "\"*** PASSED 16bit-core instruction test\""
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "\"*** FAILED 16bit-core instruction test\""
        bra     done



        end
