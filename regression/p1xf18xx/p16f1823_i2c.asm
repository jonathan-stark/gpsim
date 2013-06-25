;
; 
; Copyright (c) 2013 Roy Rankin
; MACROS and some code included the following
;************************************************************************
;*	Microchip Technology Inc. 2006
;*	02/15/06
;*	Designed to run at 20MHz
;************************************************************************
; gpsim command
.command macro x
  .direct "C", x
  endm


	;; The purpose of this program is to test gpsim's ability 
	;; to simulate a pic 16F1823.
	;; Specifically, master modes of I2C

	list		p=16F1823
	#include	p16f1823.inc
        include <coff.inc>


        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_OFF & _CLKOUTEN_OFF


#define SCL_PIN 0
#define SDA_PIN 1
#define I2CPORT PORTC
#define I2CTRI  TRISC
#define _SCL_D  I2CPORT,SCL_PIN
#define _SCL    I2CTRI,SCL_PIN
#define _SDA    I2CTRI,SDA_PIN
#define _SCL_DRIVE I2CTRI,2
#define _SDA_DRIVE I2CTRI,3

	errorlevel	-302


variables	UDATA_SHR
temp1           RES     1
temp2           RES     1


STARTUP CODE
	NOP
	goto	start
	NOP
	NOP
	NOP
PROG1 	CODE

;
start	
   .sim "break c 0x100000"
   .sim "module lib libgpsim_modules"
   .sim "module load e24xx024 ee"
   .sim "module load pu pu1"
   .sim "module load pu pu2"
   .sim "node n1"
   .sim "attach n1 portc1 pu1.pin portc3 ee.SDA" ; SDA
   .sim "node n2"
   .sim "attach n2 portc0 pu2.pin portc2 ee.SCL" ; SCL
   .sim "node n3"
   .sim "attach n3 porta0 ee.WP"
   .sim "node n4"
   .sim "attach n4 porta1 ee.A0"
   .sim "node n5"
   .sim "attach n5 porta2 ee.A1"
   .sim "node n6"
   .sim "attach n6 porta4 ee.A2"
   .sim "scope.ch0 = \"portc3\""
   .sim "scope.ch1 = \"portc2\""
   .sim "pu1.xpos = 372."
   .sim "pu1.ypos = 156."
   .sim "pu2.xpos = 252."
   .sim "pu2.ypos = 156"
   .sim "ee.xpos = 252."
   .sim "ee.ypos = 60."
   .sim "p16f1823.xpos = 72."
   .sim "p16f1823.ypos = 24."




	call	ProgInit		; Get everything running step-by-step
        call	I2CSendResult
	call	write_eeprom
	call	is_ready
	call	read_eeprom
  .command "dump e ee dump.hex"
        nop
	call	is_ready
	call	clr_eeprom
  .command "load e ee dump.hex"
	nop
	call	is_ready
	call	read_eeprom

  .assert "\"*** PASSED p16f1823 I2C test\""
	goto $






;****************************** SUBROUTINES  ****************************
;************************************************************************

START_I2C	MACRO
	banksel	SSPCON2			; Generate I2C start
	bsf	SSPCON2,SEN
	btfsc	SSPCON2,SEN
	goto	$-1	
	banksel PORTA
	ENDM

RSTART_I2C	MACRO
	banksel	SSPCON2			; Generate I2C repeat start
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	
	banksel PORTA
	ENDM

STOP_I2C	MACRO
	banksel	SSPCON2			; Generate I2C stop
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	
	banksel PORTA
	ENDM

IDLE_WAIT_I2C	MACRO
	banksel SSPCON2
	movlw	0x1f
	andwf	SSPCON2,W
	BNZ	$-3
	btfsc	SSPSTAT,R_NOT_W
	goto	$-1
	banksel PORTA
	ENDM




;****************** Initialize Registers and Variables  *****************
;************************************************************************
ProgInit

	banksel ANSELA
	clrf    ANSELA
	clrf    ANSELC
	banksel	PORTA
	clrf	PORTA			; Set all bits to zero on Port A
	banksel TRISA
	clrf	TRISA
 ;	clrf	TRISC
	bsf	_SDA_DRIVE
	


	banksel	SSPADD
	movlw	0x0C			; Set I2C baud rate to 385 kHz
	movwf	SSPADD
	movlw	0x08			; Set for I2C master mode
	movwf	SSPCON
	movlw	0x28			; Enable I2C
	movwf	SSPCON
	

	return





;************************************************************************
I2CSendResult
	banksel	SSPCON2			; Generate I2C start, bus collision
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

	bsf	SSPCON2,SEN
	banksel I2CTRI
	bcf	_SCL_DRIVE
	IDLE_WAIT_I2C
   .assert  "(pir2 & 0x08) == 0x08, \"FAILED BCL1IF for start\""
	nop
	banksel I2CTRI
	bsf	_SCL_DRIVE
	banksel PIR2
	bcf	PIR2,BCL1IF

	banksel	SSPCON2			; Generate I2C start
	bsf	SSPCON2,SEN
	IDLE_WAIT_I2C

  .assert "(portc & 0x03) == 0, \"FAILED Start SCL, SDL low\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED Start SSP1IF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED Start BCL1IF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED Start S bit set\""
	nop

	banksel PIR1
	bcf	PIR1,SSP1IF

	banksel	SSPCON2			; Generate I2C restart
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	

  .assert "(portc & 0x03) == 0, \"FAILED RStart SCL, SDL low\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED RStart SSP1IF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED RStart BCL1IF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED RStart S bit set\""
	nop

	banksel PIR1
	bcf	PIR1,SSP1IF

	banksel SSPCON2
	bsf	SSPCON2,ACKDT
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	
  .assert "(portc & 0x03) == 0x02, \"FAILED ACKEN SCL low, SDL high\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED ACKEN SSP1IF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED ACKEN BCL1IF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED ACKEN S bit set\""
	nop
	bcf	SSPCON2,ACKDT

	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSP1IF

	banksel	SSPCON2			; Generate I2C restart
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	

  .assert "(portc & 0x03) == 0, \"FAILED RStart SCL, SDL low\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED RStart SSP1IF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED RStart BCL1IF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x08, \"FAILED RStart S bit set\""
	nop

	banksel PIR1
	bcf	PIR1,SSP1IF

	banksel	SSPCON2			; Generate I2C stop
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

  .assert "(portc & 0x03) == 0x03, \"FAILED Stop SCL, SDL high\""
	nop
  .assert "(pir1 & 0x08) == 0x08, \"FAILED Stop SSP1IF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED Stop BCL1IF clear\""
	nop
  .assert "(sspstat & 0x3f) == 0x10, \"FAILED Stop P bit set\""
	nop

	return
;
;	repeatedly send command to eeprom until an ACK
;	is received back
;
;	The call of delay is not required for operation of the code,
;	but it speeds up the simulation with the GUI running. -- RRR
;
is_ready
	banksel SSPCON
	movlw	0x04
	movwf	temp2
	clrf	temp1
	call delay
	banksel	SSPCON2		; Generate I2C start
	bsf	SSPCON2,SEN
	btfsc	SSPCON2,SEN
	goto	$-1	
	movlw	0xa0		; write command to eeprom
	call	I2C_send_w
	call I2C_stop
	banksel	SSPCON2		; Generate I2C start
	btfsc	SSPCON2,ACKSTAT
	goto	is_ready
	banksel PIR2
	bcf	PIR2,BCL1IF
	return
	
write_eeprom_address
	banksel	SSPCON2		; Generate I2C start
	bsf	SSPCON2,SEN
	btfsc	SSPCON2,SEN
	goto	$-1	
	movlw	0xa0		; write command to eeprom
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write command to eeprom ACK\""
	nop
  .assert "(sspstat & 0x01) == 0x00, \"FAILED write to eeprom BF clear\""
	nop

	banksel PIR1
	bcf	PIR1,SSP1IF
	movlw	0x00		; write eeprom address
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write address to eeprom ACK\""
	nop
	return

write_eeprom
	call 	write_eeprom_address
	banksel PIR1
	bcf	PIR1,SSP1IF
	movlw	0x80		; write data1
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data1 to eeprom ACK\""
	nop

	banksel PIR1
	bcf	PIR1,SSP1IF
	movlw	0x81		; write data2
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data2 to eeprom ACK\""
	nop
	nop
	call I2C_stop
	return

clr_eeprom
	call 	write_eeprom_address
	banksel PIR1
	bcf	PIR1,SSP1IF
	movlw	0x00		; write data1
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data1 to eeprom ACK\""
	nop

	banksel PIR1
	bcf	PIR1,SSP1IF
	movlw	0x00		; write data2
	call	I2C_send_w
  .assert "(sspcon2 & 0x40) == 0x00, \"FAILED write data2 to eeprom ACK\""
	nop
	nop
	call I2C_stop
	return


read_eeprom

	call 	write_eeprom_address
	banksel	SSPCON2		; Generate I2C repeated start
	bsf	SSPCON2,RSEN
	btfsc	SSPCON2,RSEN
	goto	$-1	
	movlw	0xa1		; Send address/read to eeprom
	banksel SSPBUF
	movwf	SSPBUF
	banksel SSPCON2		; wait for idle (ACKEN,RCEN,PEN,RSEN,SEN) == 0
	movlw	0x1f
	andwf	SSPCON2,W
	BNZ	$-3
	btfsc	SSPSTAT,R_NOT_W	; also R_W == 0
	goto	$-1

  .assert "(sspstat & 0x01) == 0x00, \"FAILED address/read to eeprom BF clear\""
	nop

	banksel PIR1
	bcf	PIR1,SSP1IF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read data from eeprom
	btfsc	SSPCON2,RCEN
	goto	$-1	
  .assert "(pir1 & 0x08) == 0x08, \"FAILED RCEN SSP1IF set\""
	nop
  .assert "(pir2 & 0x08) == 0x00, \"FAILED RCEN BCL1IF clear\""
	nop
  .assert "(sspstat & 0x01) == 0x01, \"FAILED RCEN BF set\""
	nop
	banksel SSPBUF
	movf	SSPBUF,W
  .assert "W == 0x80, \"FAILED RCEN, read Data\""
	nop
  
	banksel PIR1
	bcf	PIR1,SSP1IF

	banksel	SSPCON2
	bcf	SSPCON2,ACKDT	; send ACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	banksel PIR1
	bcf	PIR1,SSP1IF

	banksel	SSPCON2
	bsf	SSPCON2,RCEN	; read next byte
	btfsc	SSPCON2,RCEN
	goto	$-1	
	banksel SSPBUF
	movf	SSPBUF,W
  .assert "W == 0x81, \"FAILED RCEN, read Data2\""
	nop

	banksel	SSPCON2
	bsf	SSPCON2,ACKDT	; send NACK
	bsf	SSPCON2,ACKEN
	btfsc	SSPCON2,ACKEN
	goto	$-1	

	return


I2C_stop
	banksel	SSPCON2			; Generate I2C stop
	bsf	SSPCON2,PEN
	btfsc	SSPCON2,PEN
	goto	$-1	

	return
	
delay
        decfsz  temp1,f
         goto   $+2
        decfsz  temp2,f
         goto   delay
        return

;********************** Output byte in W via I2C bus ********************
;************************************************************************
I2C_send_w
	banksel	SSPBUF			; Second byte of data (middle)
	movwf	SSPBUF
	banksel	SSPSTAT
	btfsc	SSPSTAT,R_NOT_W
	goto	$-1

	return
	




	end	
