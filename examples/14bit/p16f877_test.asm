
	list	p=16f877
  __config _wdt_off

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16f877.
	;; (this was derived from the similar program for testing the c64).
	
include "p16f877.inc"
		
  cblock  0x20

	x
	t1,t2,t3
  endc


	
  org	0
	goto	main


  org	4
	retfie

main:
	
	;; clear ram
	movlw	0
	call	preset_ram
	movlw	0x55
	call	preset_ram
	movlw	0
	call	preset_ram
	
	;; use the indirect register to increment register 0x70.
	;; This register is aliased in all four banks.
	;; Then use direct addressing to verify that the increment
	;; occurred. Note that bank switching is necessary to access
	;; 0x70 directly in the other banks (that is to access 0xf0,
	;; 0x170 and 0x1f0 directly, you need to switch to bank 1,2
	;; or 3 respectively).
	
	bcf	status,rp0
	movlw	0x70
	movwf	fsr

	clrf	0x70
	incf	indf,f
	btfss	0x70,0
	 goto	$-1

	bsf	status,rp0	;bank 1
	incf	indf,f
	btfsc	0x70,0
	 goto	$-1

	bsf	status,rp1	;bank 3
	incf	indf,f
	btfss	0x70,0
	 goto	$-1

	bcf	status,rp0	;bank 2
	incf	indf,f
	btfsc	0x70,0
	 goto	$-1

	bcf	status,rp1	;bank 0


	;; disable (primarily) global and peripheral interrupts
	
	clrf	intcon

	goto	pwm_test1

	;;
	;; TMR1 test
	;;

	;; Clear all of the bits of the TMR1 control register:
	;; this will:
	;;	Turn the tmr off
	;;	Select Fosc/4 as the clock source
	;;	Disable the External oscillator feedback circuit
	;;	Select a 1:1 prescale
	
	clrf	t1con		;
	bcf	pir1,tmr1if	; Clear the interrupt/roll over flag

	;; Zero the TMR1

	clrf	tmr1l
	clrf	tmr1h

	;; Test rollover
	;;  the following block of code will test tmr1's rollover for
	;; each of the 4 prescale values.
	
	clrf	x

	;; Start the timer

	bsf	t1con,tmr1on

tmr1_test1:


	;; Loop until the timer rolls over:

	btfss	pir1,tmr1if
	 goto	$-1

	bcf	pir1,tmr1if
	
	incf	x,f
	btfss	x,2
	 goto   tmr1_test1

	clrf	x

	movf	t1con,w
	addlw   (1<<t1ckps0)
	movwf   t1con
	andlw	0x40
	skpnz
	 goto   tmr1_test1
	
	clrf	t1con

	;;
	;; TMR2 test
	;;

tmr2_test:

	clrf	t2con
	movlw	0x40
	movwf	t1
	clrf	x

tmr2_test1:

	movf	t1,w
	bsf	status,rp0
	movwf	pr2
	bcf	status,rp0

	btfss	t2con,tmr2on
	 bsf	t2con,tmr2on
	
	bcf	pir1,tmr2if
	
	btfss	pir1,tmr2if
	 goto	$-1

	movlw	0x40
	addwf	t1,f

	incf	x,f
	btfss	x,2
	 goto	tmr2_test1

	goto	$

tmr1_test2:	
	bsf	status,rp0
 	bsf	portc,2		;CCP bit is an input
	bcf	status,rp0

	movlw	7
	movwf	ccp1con

	;; Start the timer

	bsf	t1con,tmr1on

tt2:	
	movf	portc,w
	decfsz	x,f
	 goto	$-1
	
	goto	tt2	
	goto	main

tmr1_test3:
	bsf	status,rp0
 	bcf	portc,2		;CCP bit is an output
	bcf	status,rp0

	movlw	0xb
	movwf	ccp1con

	;; Initialize the 16-bit compare register:

	movlw	0x34
	movwf	ccpr1l
	movlw	0x12
	movwf	ccpr1h

tt3:
	;;
	bcf	pir1,ccp1if
	
	;; Try the next capture mode
;; 	incf	ccp1con,f	

	;; If bit 2 is set then we're through with capture modes
	;; (and are starting pwm modes)

	btfsc	ccp1con,2
	 goto	die

	;; Start the timer

	bsf	t1con,tmr1on


	btfss	pir1,ccp1if
	 goto	$-1
	goto	tt3

pwm_test1:
	bsf	status,rp0

 	bsf	portc,2		;CCP bit is an input

	movlw	0x7f
	movwf	pr2

	bcf	status,rp0

	movlw	0x40
	movwf	ccpr1l
	clrf	ccpr1h
	movlw	4
	movwf	t2con
	movlw	0xc
	movwf	ccp1con
	
die:

	goto	$


preset_ram
	clrf	status

	movwf	0x20
	movlw	0x20
	movwf	fsr
	movf	0x20,w
cb0:	
	movwf	indf
	incf	fsr,f
	btfss	fsr,7
	 goto	cb0
	bsf	fsr,5
cb1:	
	movwf	indf
	incfsz	fsr,f
	 goto	cb1
	bsf	fsr,4
	bsf	status,7
cb2:	
	movwf	indf
	incf	fsr,f
	btfss	fsr,7
	 goto	cb2
	bsf	fsr,5
cb3:	
	movwf	indf
	incfsz	fsr,f
	 goto	cb3

	clrf	status

	return
	
	end