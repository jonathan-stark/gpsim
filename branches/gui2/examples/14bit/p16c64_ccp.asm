
	;; gpasm bug --- c64 is not recognized...
	
	list	p=16c84
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate the
	;; Capture Compare peripherals in a midrange pic (e.g. pic16c64). See
	;; p16c64_tmr1.asm for additional tmr1 examples.
	;; NOTE: The capture mode will only work if there is something to capture!
	;; Hence this file should loaded into gpsim with the startup command file
	;; p16c64_ccp.stc - this will give you a square wave that ccp can capture.

include "p16c64.inc"

  cblock  0x20
	status_temp,w_temp
	interrupt_temp

	temp1,temp2
	t1,t2,t3
	kz
  endc

  cblock 0xa0
	status_temp_alias,w_temp_alias
  endc

	
  org	0
	goto	main

  org	4
		
	;; 
	;; Interrupt
	;;

	movwf	w_temp
	swapf	status,w
	movwf	status_temp

	;; Are peripheral interrupts enabled?
	btfss	intcon,peie
	 goto	exit_int

	bsf	status,rp0
	movf	pie1,w
	bcf	status,rp0
	movwf	interrupt_temp

check_tmr1:
	btfsc	pir1,tmr1if
	 btfss	interrupt_temp,tmr1ie
	  goto	check_ccp1

    ;; tmr1 has rolled over
	
	bcf	pir1,tmr1if	; Clear the pending interrupt
	bsf	temp1,0		; Set a flag to indicate rollover

check_ccp1:
	btfsc	pir1,ccp1if
	 btfss	interrupt_temp,ccp1ie
	  goto	exit_int

	bcf	pir1,ccp1if	; Clear the pending interrupt
	bsf	temp1,1		; Set a flag to indicate match

exit_int:		

	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w

	retfie


main:

	clrf	kz		;kz == Known Zero.

	;; disable (primarily) global and peripheral interrupts
	
	clrf	intcon

	;;
	;; CCP test
	;;

	;; The CCP module is intricately intertwined with the TMR1
	;; module. So first, let's initialize TMR1:

	;; Clear all of the bits of the TMR1 control register:
	;; this will:
	;;	Turn the tmr off
	;;	Select Fosc/4 as the clock source
	;;	Disable the External oscillator feedback circuit
	;;	Select a 1:1 prescale
	
	clrf	t1con		;
	clrf	pir1		; Clear the interrupt/roll over flag

	;; Zero the TMR1

	clrf	tmr1l
	clrf	tmr1h

	;; Start the timer

	bsf	t1con,tmr1on


ccp_test1:
	movlw	(1<<gie) | (1<<peie)
	movwf	intcon

	bsf	status,rp0
 	bsf	portc,2		;CCP bit is an input
	bsf	pie1,ccp1ie
	bcf	status,rp0

	;;
	;; Start the capture mode that captures every falling edge
	;; (please refer to the mchip documentation on the details
	;; of the various ccp modes.)
	;; ccp = 4  <- capture every falling edge
	;; ccp = 5  <- capture every rising edge
	;; ccp = 6  <- capture every 4th rising edge
	;; ccp = 7  <- capture every 16th rising edge
	;;
	;; Note, the capture only works if the stimulus is present
	;; on the ccp pin. (Try invoking gpsim with 'p16c64_ccp.stc'
	;; to get the proper stimulus defined.)
	
	movlw	4
	movwf	ccp1con

	;; Start the timer

	bsf	t1con,tmr1on
	clrf	t1		;A 16-bit software timeout counter
	clrf	t2

ccp_t1:

	;;
	;; when an edge is captured, the interrupt routine will
	;; set a flag:
	;; 
	btfsc	temp1,1
	 goto	ccp_next_capture_mode

	movlw	1		;This 16-bit software counter
	addwf	t1,f		;will time out if there's something wrong,
	rlf	kz,w
	addwf	t2,f
	skpc
	 goto	ccp_t1

	goto	ccp_test2	;If we get here then we haven't caught anything!
				;Either a) there's a gpsim bug or b) the stimulus
				;file is incorrect (maybe even the wrong cpu).

ccp_next_capture_mode:
	movlw	7		;if we just processed the 16th rising edge capture mode
	xorwf	ccp1con,w	;
	skpnz			;
	 goto	ccp_test2	;Then go to the next test.

	clrf	temp1
	incf	ccp1con,f	;Next mode
	goto	ccp_t1 - 2	;clear t1 and t2 too.


	;;
	;; Compare
	;;
	;; Now for the compare mode. 
ccp_test2:

	clrf	t1con
	clrf	tmr1l
	clrf	tmr1h

	bsf	status,rp0
 	bcf	portc,2		;CCP bit is an output
	bcf	status,rp0

	;; Start off the compare mode by setting the output on a compare match
	;;
	;; ccp = 8  <- Set output on match
	;; ccp = 9  <- Clear output on match
	;; ccp = 10  <- Just set the ccp1if flag, but don't change the output
	;; ccp = 11  <- Reset tmr1 on a match

	movlw	0x8
	movwf	ccp1con

	;;
	clrf	pir1
	
	;; Initialize the 16-bit compare register:

	movlw	0x34
	movwf	ccpr1l
	movlw	0x12
	movwf	ccpr1h

tt3:
	;; Stop and clear tmr1
	clrf	t1con
	clrf	tmr1l
	clrf	tmr1h

	;; Now start it
	bsf	t1con,tmr1on

	;; Wait for the interrupt routine to set the flag:
	btfss	temp1,1
	 goto	$-1

	bcf	temp1,1
	
	;; Try the next capture mode
 	incf	ccp1con,f	

	;; If bit 2 of ccp1con is set then we're through with capture modes
	;; (and are starting pwm modes)

	btfsc	ccp1con,2
	 goto	die

	goto	tt3
	
die:

	goto	$
	end