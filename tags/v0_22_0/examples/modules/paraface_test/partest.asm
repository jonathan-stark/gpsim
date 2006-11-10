	list	p=16c84
	__config _wdt_off & _rc_osc

include "p16c84.inc"

	org	0
	goto	start
	
	org	4
start
	bsf     status,rp0
	clrf portb
	bcf     status,rp0

	
   	; Every write to portb updates the parallel port data

	; When the parallel port status lines change, the
	; change will be seen on porta

begin
	incf portb,f	
	goto begin

	end

