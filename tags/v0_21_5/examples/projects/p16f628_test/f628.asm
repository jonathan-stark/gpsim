	list	p=16f628
  __config _wdt_off

	;; The purpose of this program is to test gpsim's stimulation capability
	;; A stimulus is attached to PORTB pin 0 and this program will count
	;; the rising edges that are seen on that pin.
	;;
	;;   Start gpsim:
	;;   $ gpsim
	;;   then load the Startup command file 'digital_stim.stc':	
	;;   gpsim> load c digital_stim.stc
	;;
	;;    OR
	;;
	;;   invoke gpsim with the command file
	;;   $ gpsim -c digital_stim.stc
	;;
	;;    OR
	;;
	;;   invoke the simualtion from the Makefile:
	;;   make sim

	;; In all cases, the stimulus file will load the simulation
	;; file and create the stimuli. In the Makefile case, the
	;; simulation file will be (re)created if the .asm has been
	;; been changed.
	;;
	;;



	;; The purpose of this program is to test gpsim's ability to simulate a pic 16f627/8.
	;; (this was derived from the similar program for testing the c64).
	
include "p16f628.inc"
		
  cblock  0x20

	x,y
	t1,t2,t3
	pma_string		; starting here's where a string will be copied from flash
  endc

	;; Take advantage of the upper 16 bytes all banks being aliased
  cblock 0x70

	adr_cnt
	data_cnt
	w_temp
	status_temp

  endc

DATA14  macro  _a, _b
	data (_a<<7) | (_b)
  endm
	
  org	0
	goto	main


  org	4
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	status,w
	movwf	status_temp
	
	btfss	intcon,peie
	 goto	exit_int

	bcf	status,rp1
	bcf	status,rp0

	btfss	pir1,eeif
	 goto	exit_int

;;; eeprom has interrupted
	bcf	pir1,eeif

exit_int:
	swapf	status_temp,w
	movwf	status
	swapf	w_temp,f
	swapf	w_temp,w
	retfie



main:
	
	;; clear ram
	movlw	0
	call	preset_ram
	movlw	0x55
	call	preset_ram
	movlw	0
	call	preset_ram


	;; disable (primarily) global and peripheral interrupts
	
	clrf	intcon

	call	obliterate_data_eeprom
	call	bank_access_test
	
	call	tmr1_test
 	call	tmr2_test
 	call	tmr1_test2
 	call	tmr1_test3
 	call	pwm_test1

	goto	$


	;;
	;; TMR1 test
	;;
tmr1_test:	
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

	return


	;;
	;; TMR2 test
	;;
	;; For this test, we load the PR2 register (timer2 period register)
	;; with 0x40, 0x80, 0xc0, and 0x00 and wait for TMR2 to count up
	;; to the period.
	;;

tmr2_test:

	clrf	t2con
	movlw	0x40
	movwf	t1
	clrf	x

tmr2_test1:

	movf	t1,w		;Get the next period setting
	bsf	status,rp0
	movwf	pr2		;write the new period.
	bcf	status,rp0

	btfss	t2con,tmr2on	;If tmr2 is not already on
	 bsf	t2con,tmr2on	;then turn it on
	
	bcf	pir1,tmr2if	;tmr2if is set when TMR2 finishes count
	
	btfss	pir1,tmr2if	;wait for the roll over
	 goto	$-1

	movlw	0x40		;Increase the period by 0x40 counts
	addwf	t1,f

	incf	x,f
	btfss	x,2
	 goto	tmr2_test1

	return

tmr1_test2:	
	bsf	status,rp0
; 	bsf	portc,2		;CCP bit is an input
	bcf	status,rp0

	movlw	7
	movwf	ccp1con

	;; Start the timer

	bsf	t1con,tmr1on
	clrf	y
tt2:	
;	movf	portc,w
	decfsz	x,f
	 goto	$-1
	decfsz	y,f
	  goto	tt2


	return


tmr1_test3:
	bsf	status,rp0 
;	bcf	portc,2		;CCP bit is an output
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
	clrf	status
	bsf	status,rp0

; 	bsf	portc,2		;CCP bit is an input

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

	return	
die:

	goto	$
	;; ======================================================
	;; preset_ram
	;;
	;; copy the contents of w to all of ram

preset_ram
	clrf	status

	movwf	0x20		;save the constant
	movlw	0x20		;start of ram
	movwf	fsr		;use indirect addressing to preset
	movf	0x20,w		;get the constant

	;; write to address 0x20 - 0x7f
cb0:	
	movwf	indf		;Preset...
	incf	fsr,f		;next register
	btfss	fsr,7		;loop until fsr == 0x80
	 goto	cb0

	bsf	fsr,5		;fsr = 0xa0

	;; write to address 0xa0 - 0xff
cb1:	
	movwf	indf
	incfsz	fsr,f		; loop until fsr == 0
	 goto	cb1

	bsf	fsr,5		;fsr =0x20
	bsf	status,7	;irp=1 ==> fsr =0x120

	;; write to address 0x120 - 0x14f
cb2:	
	movwf	indf
	incf	fsr,f
	
	btfsc	fsr,6		;loop until fsr == 0x50
	 btfss  fsr,4		;i.e. check for the bit pattern x1x1xxxx
	  goto	cb2

	bsf	fsr,7		;fsr == 0xd0
	bsf	fsr,5		;fsr == 0xf0

	;; write to address 0x1f0 - 0x1ff

cb3:	
	movwf	indf
	incfsz	fsr,f
	 goto	cb3

	clrf	status		;clear irp,rp0,rp1

	return


	;; =========================================================
	;; bank_access_test:
	;; 
	;; use the indirect register to increment register 0x70.
	;; This register is aliased in all four banks.
	;; Then use direct addressing to verify that the increment
	;; occurred. Note that bank switching is necessary to access
	;; 0x70 directly in the other banks (that is to access 0xf0,
	;; 0x170 and 0x1f0 directly, you need to switch to bank 1,2
	;; or 3 respectively).
bank_access_test:	
	
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

	return


;-------------------------------------------------------------
; obliterate_data_eeprom
;
; This routine will loop 255 times and write the loop iteration
;number to the entire contents of the EEPROM. I.e. on the first
;pass all 1's are written, on the second pass all 2's are written
;and so on.

obliterate_data_eeprom
	
	clrf	adr_cnt
	clrf	data_cnt
	incf    data_cnt,f

	bcf	status,rp1	;Point to bank 1
	bsf	status,rp0	;(That's where the EEPROM registers are)
	bsf	pie1,eeie	;Enable eeprom interrupts
	bsf	intcon,peie	;The peripheral interrupt bit mus also be set
				;to allow ints.

l1:	
	movf	adr_cnt,w
	movwf	eeadr
	movf	data_cnt,W
	movwf	eedata

	bcf	intcon,gie	;Disable interrupts while enabling write

	bsf	eecon1,wren	;Enable eeprom writes

	movlw	0x55		;Magic sequence to enable eeprom write
	movwf	eecon2
	movlw	0xaa
	movwf	eecon2

	bsf	eecon1,wr	;Begin eeprom write

	bsf	intcon,gie	;Re-enable interrupts
	
	btfsc	eecon1,wr	;Wait for the write to complete 
	 goto	$-1

	incf	adr_cnt,f

	btfss	adr_cnt,7	;Does the address == 0x80?
	 goto	l1

	clrf	adr_cnt

	incfsz	data_cnt,F
	 goto	l1

	return

	
		
	org	0x2100

	de	"Linux is cool!",0
	de	0xaa,0x55,0xf0,0x0f
	de	'g','p','s','i','m'
	de	"f628.asm - a program to test "
	de	"eveything an 'f877 can possibly do."
		
	end
