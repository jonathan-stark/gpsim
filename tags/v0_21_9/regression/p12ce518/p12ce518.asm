		;; 
	list	p=12ce519
        radix   dec

	nolist
include "p12ce519.inc"

	list



 __CONFIG   _CP_OFF & _IntRC_OSC & _MCLRE_OFF & _WDT_ON

  cblock  0x08
    ee_state
    eebyte
    ee_bitcnt
    eeaddr
    eedata
    failures
    temp
  endc
  
  cblock  0x27

	temp1,temp2
  endc


  cblock  0x30
        eeShadow:0x10
  endc	
	org 0

	goto	start


include "EEdriver.asm"

start:
    movlw   0xC0    ; Set up GPIO pins
    movwf   GPIO
    movlw   0x0F
    tris    GPIO
    movlw   1<<7      ; Assume that we're going to fail
    movwf   failures  ; by setting the MSB of failures

FillEEPROM:
        movlw   0
        movwf   temp
        movlw   eeShadow
        movwf   FSR

FillLoop:
        clrwdt

    ; Set the address
        movf    FSR,W
        andlw   0x0f
        movwf   eeaddr

    ; Set the data
        swapf   temp,W
        iorwf   temp,W
        movwf   eedata
        movwf   INDF

        call    write_byte

FillWaitForWrite:
        call    read_current
        btfss   ee_state,ee_ok
        goto    FillWaitForWrite

        movf    INDF,W
        xorwf   eedata,W
        skpz
         incf   failures,f      ; failed 

        incf    FSR,F
        incf    temp,F
        btfss   temp,4
         goto   FillLoop

    ; If we reach this point then it means that we didn't
    ; get stuck in the loop. 
	bcf	failures,7
done:
        goto    done


    goto    start

	end
