	;; The purpose of this program is to test gpsim's ability to simulate
	;; USART in the 14bit core.


	list	p=16c74
  __config _WDT_OFF

  radix  dec

include "p16c74.inc"

#define	RX_BUF_SIZE	0x10

  cblock  0x20

	temp1
	temp2
	temp3

	w_temp
	status_temp
	fsr_temp

	tx_ptr
	rx_ptr
	rx_buffer : RX_BUF_SIZE


  endc
	org 0

	goto	start

	org	4
	;; Interrupt
	;; 
	movwf	w_temp
	swapf	STATUS,w
	movwf	status_temp

	bcf	STATUS,RP0

	btfsc	INTCON,PEIE
	 btfss	PIR1,RCIF
	  goto	int_done

;;;
	movf	FSR,w
	movwf	fsr_temp
	incf	rx_ptr,w
	andlw	0x0f
	movwf	rx_ptr
	addlw	rx_buffer
	movwf	FSR
	movf	RCREG,w
	movwf	INDF
	movf	fsr_temp,w
	movwf	FSR
	
int_done:	
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,f
	swapf	w_temp,w
	retfie

	;; ----------------------------------------------------
	;;
	;;            start
	;; 
start	
	
	;; USART Initialization
	;;
	;; Turn on the high baud rate (BRGH), disable the transmitter,
	;; disable synchronous mode.
	;;
	
	clrf	STATUS

	bsf	STATUS,RP0

	;; CSRC - clock source is a don't care
	;; TX9  - 0 8-bit data
	;; TXEN - 0 don't enable the transmitter.
	;; SYNC - 0 Asynchronous
	;; BRGH - 1 Select high baud rate divisor
	;; TRMT - x read only
	;; TX9D - 0 not used
	
	movlw	(1<<BRGH)
	movwf	TXSTA ^ 0x80

	movlw   129		;9600 baud.
	movwf   SPBRG ^0x80

	;;clrf	SPBRG		;Highest possible baud rate.

	movlw	0x80		; Make rc6 and rc7 inputs. These are the
	movwf	TRISC^0x80	; the RX and TX pins for the USART
	
	bcf	STATUS,RP0

	clrf	tx_ptr
			
	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	RCSTA

	movf	RCREG,w          ;Clear RCIF
	bsf	INTCON,GIE
	bsf	INTCON,PEIE

	;; Delay for a moment to allow the I/O lines to settle
;	clrf	temp2
;	call	delay
	
	movf	RCREG,w          ;Clear RCIF
	movf	RCREG,w          ;Clear RCIF

	;; Enable the transmitter
	bsf	STATUS,RP0
	bsf	TXSTA^0x80,TXEN
	bsf	PIE1^0x80,RCIE
	bcf	STATUS,RP0

tx_loop:	
	call	tx_message
	movwf	TXREG

	bsf	temp2,4
	call	delay		;; Delay between bytes.

	btfss	PIR1,TXIF
	 goto	$-1

	goto	tx_loop

tx_message
	incf	tx_ptr,w
	andlw	0x0f
	movwf	tx_ptr
	addlw	TX_TABLE
	skpnc
	 incf	PCLATH,f
	movwf	PCL
TX_TABLE
	dt	"0123456789ABCDEF",0

	
	movlw	0xaa
	movwf	TXREG

	btfss	PIR1,TXIF	;Did the interrupt flag get set?
	 goto	$-1

	bsf	STATUS,RP0
	btfss	TXSTA^0x80,TRMT	;Wait 'til through transmitting
	 goto	$-1
	bcf	STATUS,RP0

;;; 	bcf	TXSTA,TXEN
	bcf	PIR1,TXIF
	bcf	PIR1,RCIF

rx_loop:

	bsf	RCSTA,CREN
	btfss	PIR1,RCIF
	 goto	$-1

	bcf	PIR1,RCIF

	goto	rx_loop

	goto $


delay	
	decfsz	temp1,f
	 goto 	$+2
	decfsz	temp2,f
	 goto   delay
	return

	end
