	list	p=16c84
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate interrupts.

	;; !!!! SPECIAL NOTES !!!!!
	;; Some of the tests require external stimuli. Please use 'ioport_stim.stc'
	
include "p16c84.inc"
		
  cblock  0x20

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
	swapf	status,w
	movwf	status_temp

	btfsc	eecon1,eeif
	 btfss	intcon,eeie
	  goto	check_rbi

    ;;  eeprom has interrupted
	bcf	eecon1,eeif

check_rbi:
	btfsc	intcon,rbif
	 btfss	intcon,rbie
	  goto	check_int

	bsf	temp5,1		;Set a flag to indicate rb4-7 int occured
	bcf	intcon,rbif
	
check_int:
	btfsc	intcon,intf
	 btfss	intcon,inte
	  goto	check_t0

	bsf	temp5,0		;Set a flag to indicate rb0 int occured
	bcf	intcon,intf

check_t0:
	btfsc	intcon,t0if
	 btfss	intcon,t0ie
	  goto	exit_int

    ;; tmr0 has rolled over
	
	bcf	intcon,t0if	; Clear the pending interrupt
	bsf	temp1,0		; Set a flag to indicate rollover
		
exit_int:		

	swapf	status_temp,w
	movwf	status
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
	
	bsf	intcon,t0ie	;Enable TMR0 overflow interrupts
	
	bsf	intcon,gie	;Global interrupts

	clrf	temp1		;Software flag used to monitor when the
				;interrupt has been serviced.

	clrw
test_tmr0:	
	option			;Assign new prescale value


	btfss   temp1,0		;Wait for the interrupt to occur and
	 goto   $-1		;get serviced

	
	clrf	temp1		;Clear flag for the next time
	
	bsf	status,rp0
	incf	option_reg,w
	bcf	status,rp0
	
	andlw	0x7		;Check the prescaler
	skpz
	 goto	test_tmr0

	bcf	intcon,t0ie	;Disable tmr0 interrupts

	;; Now check tmr0 with an external clock source
	;;
	;; It assumes that port b bit 0 is the stimulus.
	;; This requires that the following gpsim commands be invoked:
	;;  gpsim> node new_test_node
	;;  gpsim> attach new_test_node porta4 portb0

	bsf	status,rp0
	bcf	(trisb ^ 0x80),0	;portb bit 0 is an output
	bcf	status,rp0

	;; assign the prescaler to the wdt so that tmr0 counts every edge
	;; select the clock source to be tocki
	;; and capture low to high transitions (tose = 0)
	
 	movlw	(1<<t0cs) | (1<<psa)
;	movlw	(1<<t0cs) | (1<<psa) | (1<<t0se)
	option
	
	movlw	0xff
	movwf	temp2
	movwf	temp3
	movwf	temp4
	
	bcf	intcon,t0if	;not necessary..., but clear pending int.
	bsf	intcon,t0ie	;Re-enable tmr0 interrupts
	
tmr0_l1:
	bcf	temp1,0		;Interrupt flag

	clrz	
	bcf	portb,0
	btfsc	temp1,0
	 decf	temp2,f		;Falling edge caused the interrupt

	bcf	temp1,0
	bsf	portb,0
	btfsc	temp1,0
	 decf	temp3,f		;Rising edge caused the interrupt

	;; if either temp2 or temp3 decremented to zero, then z will be set
	skpz
	 goto	tmr0_l1

	incfsz	temp4,f
	 goto	test_inte
	
	;; Now let's test external clocking with the falling edge
	
	movlw	(1<<t0cs) | (1<<psa) | (1<<t0se)
	option
	
	goto	tmr0_l1

	;; tmr0 test is done.

	;;
	;; inte test
	;;
	;; The following block of code tests the interrupt on
	;; change for port b bit 0. It assumes that port a bit 4
	;; is the stimulus. This requires that the following
	;; gpsim commands be invoked:
	;;  gpsim> node new_test_node
	;;  gpsim> attach new_test_node porta4 portb0
	;;
	;; Also, recall that porta bit 4 is an open collector
	;; output (it can't drive high). So to generate the
	;; the logic highs, the portb weak pull-up resistors
	;; need to be enabled.
	
test_inte:

	
	bsf	status,rp0
	bcf	(trisa ^ 0x80),4	;Make porta bit 4 an output
	bsf	(trisb ^ 0x80),0	;and portb bit 0 an input
	bcf	option_reg,rbpu		;Enable the portb weak pull-ups
	bsf	option_reg,intedg	;Interrupt on rising edge
	bcf	status,rp0


	movlw	0xff
	movwf	temp1
	movwf	temp2
	movwf	temp3
	movwf	temp4
		
	bcf	porta,4
	bcf	intcon,rbif
	bsf	intcon,inte


	;;
	;; This routine toggles porta bit 4 with the hope of generating
	;; an interrupt on portb bit 0.
	;; temp1 counts the number of times an interrupt has occurred
	;; temp2 counts the number of times the interrupt was due to
	;;	a rising edge.
inte_edgecount:
	bcf	temp5,0		;Interrupt flag

	clrz	
	bcf	porta,4		;Falling edge

	btfsc	temp5,0
	 decf	temp2,f		;Falling edge caused the interrupt

	bcf	temp5,0
	bsf	porta,4		;Rising edge

	btfsc	temp5,0
	 decf	temp3,f		;Rising edge caused the interrupt

	;; if either temp2 or temp3 decremented to zero, then z will be set
	skpz
	 goto	inte_edgecount

	incfsz	temp4,f
	 goto	test_rbif
	
	;; Now let's test the falling edge
	
	bcf	intcon,inte		;Disable inte interrupt
	bcf	porta,4
	bcf	intcon,intf
	
	bsf	status,rp0
	bcf	option_reg,intedg	;Interrupt on falling edge
	bcf	status,rp0

	bsf	intcon,inte
	
	goto	inte_edgecount

	;;
	;; test_rbif
	;;
	;; This next block tests the interrupt on change feature of
	;; port b's I/O pins 4-7
	;; 
test_rbif

	bcf	intcon,inte	;Disable the rb0 interrupt

	bsf	status,rp0
	bsf	(trisa ^ 0x80),4	;Porta bit 4 is now an input 
	bcf	status,rp0

	clrf	temp5			;Interrupt flag
	clrf	temp1
	
	movlw	0x10
	movwf	temp2

rbif_l1:
	
	bcf	intcon,rbie
	
	movf	temp2,w
	tris	portb

	clrf	portb
	movf	portb,w

	clrf	temp5			;Interrupt flag

	bcf	intcon,rbif
	bsf	intcon,rbie


	swapf	temp2,w
	movwf	portb

	btfsc	temp5,1
	 iorwf	temp1,f

	clrc
	rlf	temp2,f

	skpc
	 goto	rbif_l1


	
	goto	$

	end
