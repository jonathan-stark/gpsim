	;; The purpose of this program is to test gpsim's ability to simulate
	;; USART in the 14bit core.


	list	p=16c65
  __config _wdt_off


include "p16c65.inc"
		
  cblock  0x20

	temp1
	temp2

	w_temp
	status_temp

  endc
	org 0

	goto	start

	org	4

start

	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	status
	bsf	status,rp0
	movlw	(1<<BRGH)
	movwf	txsta
	clrf	spbrg		;Highest possible baud rate.
	
	movlw	0x80		; Make rc6 and rc7 inputs. These are the
	movwf	trisc		; the RX and TX pins for the USART
	
	bcf	status,rp0
		
	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	rcsta

	;; Enable the transmitter
	bsf	status,rp0
	bsf	txsta,txen
	bcf	status,rp0

	movlw	0xaa
	movwf	txreg

	btfss	pir1,txif	;Did the interrupt flag get set?
	 goto	$-1

	bsf	status,rp0
	btfss	txsta,trmt	;Wait 'til through transmitting
	 goto	$-1
	bcf	status,rp0

;;; 	bcf	txsta,txen
	bcf	pir1,txif
	bcf	pir1,rcif

rx_loop:

	bsf	rcsta,cren
	btfss	pir1,rcif
	 goto	$-1

	bcf	pir1,rcif

	goto	rx_loop
	
	goto $
	
	end



