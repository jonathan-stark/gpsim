	;; The purpose of this program is to test gpsim's ability to simulate
	;; TMR0 in the 18cxxx core.

	
include "p18c242.inc"
		
  cblock  0

	temp1
	temp2
	temp3
	temp4
	temp5
	adr_cnt
	data_cnt

	w_temp
	status_temp

  endc
	org 0

	goto	start

	org	4
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	status,w,0
	movwf	status_temp

check_t0:
	btfsc	intcon,t0if,0
	 btfss	intcon,t0ie,0
	  goto	exit_int

    ;; tmr0 has rolled over
	
	bcf	intcon,t0if,0	; Clear the pending interrupt
	bsf	temp1,0		; Set a flag to indicate rollover
		
exit_int:		

	swapf	status_temp,w,0
	movwf	status,0
	swapf	w_temp,f
	swapf	w_temp,w
	retfie

start

	;;
	;; tmr0 test
	;;
	;; The following block of code tests tmr0 together with interrupts.
	;; Each prescale value (0-7) is loaded into the timer. The software
	;; waits until the interrupt due to tmr0 rollover occurs before
	;; loading the new prescale value.
	
	bsf	intcon,t0ie,0	;Enable TMR0 overflow interrupts
	
	bsf	intcon,gie,0	;Global interrupts

	clrf	temp1		;Software flag used to monitor when the
				;interrupt has been serviced.
	clrf	temp2		;Keeps track of 8 vs 16 bit mode for tmr0

	clrw
	btg	temp2,t08bit
test_tmr0:
	iorwf	temp2,w
	movwf	t0con,0		;Assign new prescale value


	btfss   temp1,0		;Wait for the interrupt to occur and
	 goto   $-1		;get serviced

	
	clrf	temp1		;Clear flag for the next time
	
	incf	t0con,w,0
	
	andlw	0x7		;Check the prescaler
	skpz
	 bra	test_tmr0

	btfsc	temp2,t08bit
	 bra	test_tmr0-1
	
	bcf	intcon,t0ie,0	;Disable tmr0 interrupts

	bra	tmr0_done

	;; Now check tmr0 with an external clock source
	;;
	;; It assumes that port b bit 0 is the stimulus.
	;; This requires that the following gpsim commands be invoked:
	;;  gpsim> node new_test_node
	;;  gpsim> attach new_test_node porta4 portb0

	bcf	trisb,0,0	;portb bit 0 is an output

	;; assign the prescaler to the wdt so that tmr0 counts every edge
	;; select the clock source to be tocki
	;; and capture low to high transitions (tose = 0)
	
 	movlw	(1<<t0cs) | (1<<psa)
	movwf	t0con,0
	
	movlw	0xff
	movwf	temp2
	movwf	temp3
	movwf	temp4
	
	bcf	intcon,t0if,0	;not necessary..., but clear pending int.
	bsf	intcon,t0ie,0	;Re-enable tmr0 interrupts
	
tmr0_l1:
	bcf	temp1,0		;Interrupt flag

	clrz	
	bcf	portb,0,0
	btfsc	temp1,0
	 decf	temp2,f		;Falling edge caused the interrupt

	bcf	temp1,0
	bsf	portb,0,0
	btfsc	temp1,0
	 decf	temp3,f		;Rising edge caused the interrupt

	;; if either temp2 or temp3 decremented to zero, then z will be set
	bnz	tmr0_l1

	incfsz	temp4,f
	 bra	tmr0_done
	
	;; Now let's test external clocking with the falling edge
	
	movlw	(1<<t0cs) | (1<<psa) | (1<<t0se)
	movwf	t0con,0
	
	bra	tmr0_l1

	;; tmr0 test is done.
tmr0_done:
	bra	$
	
	end
