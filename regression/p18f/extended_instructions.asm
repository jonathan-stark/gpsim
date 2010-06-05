        ;; it.asm
        ;;
        ;; The purpose of this program is to test how well gpsim can simulate
        ;; the 18f extended instructions.


	list    p=18f452                ; list directive to define processor
	include <p18f452.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

	radix dec

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
failures        RES     1


dataStack	RES	32

  GLOBAL done
  GLOBAL temp1
  GLOBAL dataStack

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE

	bra	start
  ;------------------------------------------------------------
  ; function vectors
  ; The function vectors are branch instructions to functions
  ; that reside at addresses for which PC>256. They're used to
  ; make indirect calls with the extended CALLW instruction.

vf1	bra	f1
vf2	bra	f2
vf3	bra	f3
vf4	bra	f4
vf5	bra	f5

start:  

        clrf    temp1           ;Assume clrf works...
                                ;
	clrf	FSR0
	clrf	FSR1
	clrf	FSR2
	clrf	BSR

	addfsr	0,15
   .assert "(fsr0l==15) && (fsr0h==0)"
	nop
	addfsr	1,15
   .assert "(fsr1l==15) && (fsr1h==0)"
	nop
	addfsr	2,15
   .assert "(fsr2l==15) && (fsr2h==0)"
	nop


	subfsr	0,10
   .assert "(fsr0l==5) && (fsr0h==0)"
	nop
	subfsr	1,10
   .assert "(fsr1l==5) && (fsr1h==0)"
	nop
	subfsr	2,10
   .assert "(fsr2l==5) && (fsr2h==0)"
	nop


   ;; CALLW test
   ;; This test makes 5 indirect calls using the callw instruction.
   ;; There is a function vector table in low memory that redirects
   ;; calls to high memory. The reason for this table is to allow the
   ;; destination address for the CALLW instruction to fit entirely
   ;; in the WREG

	clrf	PCLATU
	clrf	PCLATH
	movlw	5
   .assert "temp1==0"
	movwf	temp1		;temp1 is used as the index into 
	nop			;function vector table.

callw_loop:
	rlncf	temp1,W		;program memory indexes through even addresses
	addlw	vf1-2		;offset to the start of the function vectors.
	callw			;Make the indirect call
	decfsz  temp1,F		;next vector
	 bra	callw_loop


    ;;
	lfsr	2, dataStack+10
	clrf	dataStack
	pushl	0x42
   .assert "(fsr2l == ((&dataStack)+10-1))"

	nop

	movsf	[1],WREG
   .assert "W == 0x42"

	movss	[1],[3]

   .assert "(*(fsr2l+1))==(*(fsr2l+3))"
        bra     done

;------------------------------------------------------------
; CALLW functions - here are 5 functions that are called indirectly
; by the callw instruction. All they do is return. However, the assertion
; verifies that the index (temp1) used to calculate the address of the
; indirect call is correct.
f1:
   .assert "temp1==1"
	return
f2:
   .assert "temp1==2"
	return
f3:
   .assert "temp1==3"
	return
f4:
   .assert "temp1==4"
	return
f5:
   .assert "temp1==5"
	return


failed1:        ; a relatively local label
        bra     failed

        ;; Perform some basic tests on some important instructions
done:
  .assert  "\"*** PASSED 16bit-core extended instruction test\""
        bra     $

failed:
        movlw   1
        movwf   failures
  .assert  "\"*** FAILED 16bit-core extended instruction test\""
        bra     done



        end
