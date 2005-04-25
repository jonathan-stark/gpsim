		;; 
	list	p=12c509
        radix   dec

	nolist
include "p12c509.inc"

	list



  __CONFIG _WDT_ON

  cblock  0x07

	temp,temp1,temp2
  endc
	
	org 0

start:

	call	delay


   ; Intialize the OPTION register so that TMR0 counts
   ; every instruction cycle

	MOVLW	(1<<NOT_GPWU) | (1<<NOT_GPPU) | (1<<PSA)
	OPTION

	call	delay

	SLEEP

delay:
	CLRF	temp

delay_loop:
	DECFSZ	temp,F
	 goto	delay_loop

	RETURN

	goto	start

	
	end
