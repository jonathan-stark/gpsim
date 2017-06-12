
	list	p=16f871
        include <p16f871.inc>
        include <coff.inc>

  __CONFIG _WDT_OFF


        ;; The purpose of this program is to test gpsim's ability to simulate
        ;; the Parallel Slave port (PSP)  functionality.
        ;;
        ;; Portb and portd form the PSP bus and porta pins 0, 1, and 2
        ;; drive the, active low, bus control signals RD, WR and CS respectively


        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

x  RES  1
t1 RES  1
t2 RES  1
avg_lo RES  1
avg_hi RES  1
w_temp RES  1
status_temp RES  1


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program

	;; 
	;; Interrupt
	;; 
INT_VECTOR   CODE    0x004               ; interrupt vector location
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp

	bcf	STATUS,RP0	;bank 0

	btfsc	PIR1,PSPIF
	goto	done

  .assert "\"FAILED 16F871 unexpected interupt\""
	nop
check:
	swapf	status_temp,w
	movwf	STATUS
	swapf	w_temp,F
	swapf	w_temp,W
	retfie



;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start:

   .sim "node pspRD"
   .sim "attach pspRD porta0 porte0"
   .sim "node pspWR"
   .sim "attach pspWR porta1 porte1"
   .sim "node pspCS"
   .sim "attach pspCS porta2 porte2"
   .sim "node p0"
   .sim "attach p0 portb0 portd0"
   .sim "node p1"
   .sim "attach p1 portb1 portd1"
   .sim "node p2"
   .sim "attach p2 portb2 portd2"
   .sim "node p3"
   .sim "attach p3 portb3 portd3"
   .sim "node p4"
   .sim "attach p4 portb4 portd4"
   .sim "node p5"
   .sim "attach p5 portb5 portd5"
   .sim "node p6"
   .sim "attach p6 portb6 portd6"
   .sim "node p7"
   .sim "attach p7 portb7 portd7"

;
; The test relies on porta being digital I/O, and porte being PSP, so it's 
; necessary to disable all ADC channels
	bsf     STATUS,RP0      ; bank 1
	movlw	0x07
	movwf	ADCON1
	bcf	STATUS,RP0      ; bank 0


;
; First test portd operates in normal mode

	bsf     STATUS,RP0      ; bank 1
	clrf    TRISD           ; PortD output
	bcf	STATUS,RP0      ; bank 0
	movlw	0x55
	movwf	PORTD
  .assert "portd == 0x55, \"FAILED, Portd put value OK\""
	nop
  .assert "portb == 0x55, \"FAILED, Portd drives Portb\""
	nop
	movf	PORTD,W		; check read Portd
  .assert "W == 0x55, \"FAILED, Read portd OK\""
	nop
	bsf     STATUS,RP0      ; bank 1
	clrf    TRISB           ; PortB output
	movlw	0xff
	movwf	TRISD		; PortD input
	bcf	STATUS,RP0      ; bank 0
	movwf	PORTB
  .assert "portd == 0xff, \"FAILED, Read portd as input\""
	nop


;
; Now test PSP in bus RD mode
;
	movlw	0x07		; set bits high
	movwf	PORTA
	bsf	STATUS,RP0	; bank 1
	movlw   0xff
	movwf   TRISB
        movlw   0xf8
        movwf   TRISA
	movlw	0x17
	movwf	TRISE
	bcf	STATUS,RP0	; bank 0
	movlw	0xf0
	movwf	PORTD		; Output should not change in PSP mode
  .assert "(trise & 0xe0) == 0x40, \"FAILED, OBF set on portd write\""
	nop
  .assert "portb != 0xf0, \"FAILED, Output not changed\""
	nop
	bcf	PORTA,0		; Drive RD low to put data on bus
	bcf	PORTA,2		; Drive CS low to select
  .assert "(trise & 0xe0) == 0x00, \"FAILED, OBF cleared on PSP RD\""
	nop
  .assert "portb == 0xf0, \"FAILED, Output on bus\""
	nop
  .assert "(pir1 & 0x80) == 0, \"FAILED, PSPIF not set until RD off\""
	nop
	bsf	PORTA,0		; Turn off RD
  .assert "(pir1 & 0x80) == 0x80, \"FAILED, PSPIF set for RD\""
	nop
;
; test PSP WR function
;
	bsf	STATUS,RP0      ; bank 1
	clrf	TRISB		; set B as output
	bcf	STATUS,RP0      ; bank 0
	movlw	0x0f
	movwf	PORTB		; drive bus
	bcf	PORTA,1		; Turn on WR (CS already on)
  .assert "(trise & 0xe0) == 0x00, \"FAILED, IBF not set yet\""
	nop
	bsf	PORTA,2		; Turn off CS
  .assert "(trise & 0xe0) == 0x80, \"FAILED, IBF now set\""
	nop
	movf	PORTD,W
  .assert "W == 0x0f, \"FAILED, Value read from bus\""
	nop
  .assert "(trise & 0xe0) == 0x00, \"FAILED, IBF cleared on read of portd\""
	nop
	bcf	PORTA,2		; Turn on CS
	bsf	PORTA,2		; Turn off CS
	bcf	PORTA,2		; Turn on CS
	bsf	PORTA,2		; Turn off CS
  .assert "(trise & 0xe0) == 0xa0, \"FAILED, IBF, IBOV both set \""
	nop
                                                                                
	bsf	STATUS,RP0      ; bank 1
	movlw   0x17
	movwf	TRISE		; set B as output
  .assert "(trise & 0xe0) == 0x80, \"FAILED, IBOV cleared, but not IBF \""
	nop
	movlw	0x07		; turn off PSP
	movwf	TRISE		; set B as output
  .assert "(trise & 0xf0) == 0x00, \"FAILED, IBF & PSPMODE claered\""
	nop
;
; test interupts
;
	movlw	0x17		
	movwf	TRISE
	bsf	PIE1,PSPIE
	movlw	0xc0
	movwf	INTCON		; Enable interupts
	bcf	STATUS,RP0      ; bank 0

	bcf	PIR1,PSPIF
	bcf	PORTA,2		; Turn on CS
	bsf	PORTA,2		; Turn off CS
   .assert "\"FAILED, interrupt\""
	nop
	goto $-1
	


done:
  .assert  "\"*** PASSED 16F871 PSP test\""
	
	goto	$-1


	end
