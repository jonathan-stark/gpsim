	;; The purpose of this program is to test gpsim's ability to simulate
	;; USART in the 14bit core.


	list	p=16c74
  __config _wdt_off


include "p16c74.inc"

#define	RX_BUF_SIZE	0x10

  cblock  0x20

	temp1
	temp2

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
	swapf	status,w
	movwf	status_temp

	bcf	status,rp0

	btfsc	intcon,peie
	 btfss	pir1,rcif
	  goto	int_done

;;;
	movf	fsr,w
	movwf	fsr_temp
	incf	rx_ptr,w
	andlw	0x0f
	movwf	rx_ptr
	addlw	rx_buffer
	movwf	fsr
	movf	rcreg,w
	movwf	indf
	movf	fsr_temp,w
	movwf	fsr
	
int_done:	
	swapf	status_temp,w
	movwf	status
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
	
	clrf	status

	bsf	status,rp0
	
	movlw	(1<<BRGH)

	movwf	txsta
	clrf	spbrg		;Highest possible baud rate.

	movlw	0x80		; Make rc6 and rc7 inputs. These are the
	movwf	trisc		; the RX and TX pins for the USART
	
	bcf	status,rp0

	clrf	tx_ptr
			
	;; Turn on the serial port
	movlw	(1<<SPEN) | (1<<CREN)
	movwf	rcsta

	movf	rcreg,w
	bsf	intcon,gie
	bsf	intcon,peie
	
	;; Enable the transmitter
	bsf	status,rp0
	bsf	txsta,txen
	bsf	pie1,rcie
	bcf	status,rp0

tx_loop:	
	call	tx_message
	movwf	txreg

	btfss	pir1,txif
	 goto	$-1


	goto	tx_loop

tx_message
	incf	tx_ptr,w
	andlw	0x0f
	movwf	tx_ptr
	addlw	TX_TABLE
	skpnc
	 incf	PCLATH,f
	movwf	pcl
TX_TABLE
	dt	"0123456789ABCDEF",0

	
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