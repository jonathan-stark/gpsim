	list	p=16c84


	;; The purpose of this program is to test gpsim's ability to
	;; handle the wdt and the sleep mode.
	
	
include "p16c84.inc"

  __config _wdt_on

	
  cblock  0x20

	temp1
	temp2
  endc
	
	org	0

	goto begin


	org	4


begin
	;; Determine the type of reset that occurred
	btfss	status,pd
	 goto   awakened

	btfsc	status,to
	 goto	por_reset


wdt_reset			; to:pd = 01 wdt reset
	;; wdt timed out while the processor was not sleeping.
	;; A cause for concern if you're doing something real.

	goto	por_reset	; the Processor may be in a bad
				; state, so just do the por stuff.
	
por_reset			;  to:pd = 11 por reset
	;; Power on reset OR perhaps one more other kinds of resets
	;; that executed code that cleared the wdt. In either case,
	;; the processor wasn't sleeping when the reset occurred.

	;; Let's go to sleep and let the wdt time out and wake us
	nop
	sleep
	nop
	
awakened
	;; pd is clear which means that the processor was sleeping
	;; when the reset occurred.

	btfss	status,to
	 goto	awakened_by_wdt	; to:pd = 00 which means the wdt woke us.

	nop			; to:pd = 10 which means an /MCLR
	nop			; occurred while we were sleeping.
				; for now, do nothing.
	
awakened_by_wdt
	clrwdt
	
	;; to:pd = 00 which means the wdt woke us.
	
	;; If we get here, then a wdt time out occured while we
	;; were sleeping.

	;; Now, let's loop here and let the wdt break us out
	nop
	goto	$
	
wait_for_wdt
	decfsz	temp1,f
	 goto   wait_for_wdt
	
	clrwdt
	
	decfsz  temp2,f
	 goto   begin

	;; The wdt is enabled. So if sit in a loop without clearing it,
	;; eventually a reset will occur and will bring us out of the coma
		
	goto	$		;Wait for wdt to timeout

	end