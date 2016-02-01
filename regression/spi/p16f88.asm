
	list	p=16f88
        include <p16f88.inc>
        include <coff.inc>

        __CONFIG  _CONFIG1, _CP_OFF & _WDT_OFF &  _INTRC_IO & _PWRTE_ON & _LVP_OFF & _BODEN_OFF & _MCLR_OFF
        __CONFIG    _CONFIG2, _IESO_OFF & _FCMEN_OFF

	;; The purpose of this program is to test gpsim's ability to simulate a pic 16F88.
	;; Specifically, I2C

        errorlevel -302 

; Printf Command
.command macro x
  .direct "C", x
  endm


#define SDO_PORT PORTB,2
#define SDI_PORT PORTB,1
#define SCK_PORT PORTB,4
#define SS_PORT PORTB,5

#define SDO_TRIS TRISB,2
#define SDI_TRIS TRISB,1
#define SCK_TRIS TRISB,4
#define SS_TRIS TRISB,5

#define DRV_CLOCK PORTA,1
#define DRV_CLOCK_TRIS TRISA,1

#define DRV_SS PORTA,2
#define DRV_SS_TRIS TRISA,2


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR

w_temp RES  1
status_temp RES  1
loopcnt	RES	1



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
	movwf	w_temp
	swapf	STATUS,W
	movwf	status_temp
	bcf	STATUS,RP0	;adcon0 is in bank 0

        btfss   INTCON,SSPIF
        goto    check	                ; other Interrupts
;        bsf     _TIME_OUT_              ; MUST set this Flag
        bcf     INTCON,SSPIF


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

   .sim "module lib libgpsim_modules"
   .sim "module load pu pu1"
   .sim "module load pu pu2"
   .sim "module load not not"

   .sim "p16f88.xpos = 72"
   .sim "p16f88.ypos = 24"

   .sim "pu1.xpos = 228"
   .sim "pu1.ypos = 24"

   .sim "pu2.xpos = 96"
   .sim "pu2.ypos = 240"

   .sim "not.xpos = 96"
   .sim "not.ypos = 168"

   .sim "node ss"
   .sim "attach ss portb5 porta2"	; Not SS
   .sim "node sck"
   .sim "attach sck portb4 porta1 pu1.pin"	; SCK
   .sim "node sdo"
   .sim "attach sdo not.in0 portb2 pu2.pin"
   .sim "node sdi"
   .sim "attach sdi portb1 not.out"

    bsf  STATUS,RP0	; bank 1
    movlw	0xf6	; set internal RC to 8 Mhz
    movwf	OSCCON
    clrf	ANSEL
    bcf		SDO_TRIS	; SDO
    bcf		SCK_TRIS	; SCK
    bcf		DRV_SS_TRIS
    movlw	0xff
    movwf	SSPSTAT
  .assert "sspstat == 0xC0, \"*** FAILED p16f88 SPI sspstat only SMP, CKE writable\""
    clrf	SSPSTAT

    BANKSEL	SSPCON
    bsf 	DRV_SS		; set SS high

;
;  	Test SPI Master mode
;
    movlw	0x21	; SSPEN | SPI master Fosc/16
    movwf	SSPCON
    bcf		PIR1,SSPIF
    movlw	0xab
    movwf	SSPBUF

loop:
    btfss	PIR1,SSPIF
    goto	loop

  .assert "(sspstat & 1) == 1, \"*** FAILED p16f88 SSP SPI Master BF not set\""
    nop
    movf	SSPBUF,W
  .assert "(sspstat & 1) == 0, \"*** FAILED p16f88 SSP SPI Master BF not cleared\""
    nop
  .assert "W == 0x54, \"*** FAILED p16f88 SSP SPI Master wrong data\""
    nop

;
;	TEST SPI Slave mode with SS
;
    clrf	SSPCON
    bsf  STATUS,RP0	; bank 1
    bcf		DRV_CLOCK_TRIS 	; external SCK drive
    bsf		SCK_TRIS	; SCK
    bsf		SS_TRIS 	; SS
    bcf  STATUS,RP0	; bank 0
    bcf		DRV_SS
    bcf		DRV_CLOCK
    movlw	0x24	; SSPEN | SPI slave mode SS enable
    movwf	SSPCON
    bcf		PIR1,SSPIF
    movlw	0xab
    movwf	SSPBUF
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    movwf	SSPBUF	; test WCOL set
  .assert "(sspcon & 0x80) == 0x80, \"***FAILED p16f88 SSP SPI WCOL set\""
    nop
    bcf		SSPCON,WCOL	; clear WCOL bit
  .assert "(sspcon & 0x80) == 0x00, \"***FAILED p16f88 SSP SPI WCOL was cleared\""
    nop
    ; SS is high, so transfer should not happen
    bsf		DRV_SS
    movlw	0x0a
    movwf	loopcnt

ss_loop:
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    decfsz	loopcnt
    goto	ss_loop

  .assert "(pir1 & 0x08) == 0, \"*** FAILED p16f88 SSP SPI slave with SS\""
    nop
    bcf		DRV_SS		; clear SS
    

    clrf	loopcnt
loop2:
    incf	loopcnt,F
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    btfss	PIR1,SSPIF
    goto	loop2

    movf	SSPBUF,W
  .assert "W == 0x54, \"*** FAILED p16f88 SSP SPI Slave data\""
    nop
;
;	Test Slave receive overrun
;
   movlw	0x10
   movwf	loopcnt
loop4:
    bsf		DRV_CLOCK
    bcf		DRV_CLOCK
    decfsz	loopcnt,F
    goto	loop4
  .assert "(sspcon & 0x40) == 0x40, \"*** FAILED p16f88 SSP SPI SSPOV\""
    nop

;
;  	Test SPI Master mode TMR2
;
    clrf	SSPCON
    bsf  STATUS,RP0	; bank 1
    bsf		DRV_CLOCK_TRIS	; external SCK drive off
    bcf		SCK_TRIS	; SCK output
    movlw	0x1
    movwf	PR2
    bcf  STATUS,RP0	; bank 0
    clrf	TMR2
    movlw	0x3C	; prescale = 1 postscale 16
    movwf	T2CON

    movlw	0x23	; SSPEN | SPI master TMR2
    movwf	SSPCON
    bcf		PIR1,SSPIF
    movlw	0xab
    movwf	SSPBUF

loop3:
    btfss	PIR1,SSPIF
    goto	loop3

  .assert "(sspstat & 1) == 1, \"*** FAILED p16f88 SSP SPI Master TMR2, BF not set\""
    nop
    movf	SSPBUF,W
  .assert "(sspstat & 1) == 0, \"*** FAILED p16f88 SSP SPI Master TMR2, BF not cleared\""
    nop
  .assert "W == 0x54, \"*** FAILED p16f88 SSP SPI Master TMR2 wrong data\""
    nop

  .assert "\"*** PASSED 16f88 SSP SPI test\""
    
    goto $

	end
