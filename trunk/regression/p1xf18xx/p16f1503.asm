;
; 
; Copyright (c) 2017 Roy Rankin
;
; This file is part of the gpsim regression tests
; 
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2.1 of the License, or (at your option) any later version.
; 
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.
; 
; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, see 
; <http://www.gnu.org/licenses/lgpl-2.1.html>.


        ;; The purpose of this program is to test gpsim's ability to 
        ;; simulate a pic 16F1503.
        ;; Specifically, basic port operation, eerom, interrupts,
	;; a2d, dac, SR latch, capacitor sense, and enhanced instructions


	list    p=16f1503                ; list directive to define processor
	include <p16f1503.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF & _MCLRE_OFF & _CLKOUTEN_OFF
;;        __CONFIG _CONFIG1, _CP_OFF & _WDTE_ON &  _FOSC_INTOSC & _PWRTE_ON &  _BOREN_OFF &  _CLKOUTEN_OFF
        __CONFIG _CONFIG2, _STVREN_ON ; & _WRT_BOOT

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
cmif_cnt	RES	1
tmr0_cnt	RES	1
tmr1_cnt	RES	1
eerom_cnt	RES	1
adr_cnt		RES	1
data_cnt	RES	1
inte_cnt	RES	1
iocaf_val	RES	1

 GLOBAL inte_cnt, iocaf_val


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlp  high  start               ; load upper byte of 'start' label
        goto   start                     ; go to beginning of program


  .sim "module library libgpsim_modules"
  ; Use a pullup resistor as a voltage source
  .sim "module load pullup V1"
  .sim "V1.resistance = 10000.0"
  .sim "V1.capacitance = 20e-12"
  .sim "V1.voltage=1.0"

  ; Use a pullup resistor as a voltage source
  .sim "module load pullup V2"
  .sim "V2.resistance = 10000.0"
  .sim "V2.capacitance = 20e-12"
  .sim "V2.voltage=2.5"

  .sim "node n1"
  .sim "attach n1 porta0 porta3 V2.pin"
  .sim "node n2"
  .sim "attach n2 porta1 porta4"
  .sim "node n3"
  .sim "attach n3 porta2 porta5"
  .sim "node n4"
  .sim "attach n4 portc0 V1.pin"

  .sim "p16f1503.xpos = 72"
  .sim "p16f1503.ypos = 72"

  .sim "V1.xpos = 216"
  .sim "V1.ypos = 120"

  .sim "V2.xpos = 216"
  .sim "V2.ypos = 60"


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
	; many of the core registers now saved and restored automatically
                                                                                
	clrf	BSR		; set bank 0

;	btfsc	PIR2,EEIF
;	    goto ee_int

  	btfsc	INTCON,T0IF
	    goto tmr0_int

  	btfsc	INTCON,IOCIF
	    goto inte_int

	.assert "\"***FAILED p16f1503 unexpected interrupt\""
	nop


; Interrupt from TMR0
tmr0_int
	incf	tmr0_cnt,F
	bcf 	INTCON,T0IF
	goto	exit_int

; Interrupt from eerom
ee_int
	incf	eerom_cnt,F
;	bcf 	PIR2,EEIF
	goto	exit_int

; Interrupt from INT pin
inte_int
	incf	inte_cnt,F
	bcf	INTCON,GIE	; stop interrupts
	goto	exit_int

exit_int:
                                                                                
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
   .assert "p16f1503.frequency == 500000., \"FALIED 16F1503 POR clock 500kHz\""
	nop

	BANKSEL OSCCON
	bsf 	OSCCON,6	;set clock to 16 MHz
	btfss	OSCSTAT,HFIOFS
	goto	$-1
   .assert "(oscstat & 0x1f) == 0x19,  \"*** FAILED 16f1503 HFIO bit error\""
	nop
   .assert "p16f1503.frequency == 16000000., \"FALIED 16F1503 RC clock 16MHz\""
	nop
	BANKSEL FVRCON
	; Enable Core Temp, high range, FVR AD 2.048v 
	movlw	(1<<TSEN)|(1<<TSRNG)|(1<<FVREN)|(1<<ADFVR1)
	movwf	FVRCON
	BANKSEL STKPTR
	movf	STKPTR,W
	;
	; Check PIR bits writable
	BANKSEL PIR1
	movlw	0xff
	movwf	PIR1
   .assert "pir1 == 0xcb,  \"*** FAILED 16f1503  PIR1 writable bits\""
	nop
	clrf	PIR1
	movwf	PIR2
   .assert "pir2 == 0x6c,  \"*** FAILED 16f1503  PIR2 writable bits\""
	nop
	clrf	PIR2
	;
	; Check PIE bits writable
	BANKSEL PIE1
	movlw	0xff
	movwf	PIE1
   .assert "pie1 == 0xcb,  \"*** FAILED 16f1503  PIE1 writable bits\""
	nop
	clrf	PIE1
	movwf	PIE2
   .assert "pie2 == 0x6c,  \"*** FAILED 16f1503  PIE2 writable bits\""
	nop
	clrf	PIE2
;	;
	; test pins in analog mode return 0 on register read
	BANKSEL TRISA
	clrf	STATUS
	clrf	TRISA
   .assert "trisa == 0x08, \"*** FAILED 16f1503  TRISA not clear except bit 3\""
	nop
	BANKSEL PORTA
	movlw	0xff
	movwf	PORTA
   .assert "porta == 0x28, \"*** FAILED 16f1503  analog bits read 0\""
	nop
	movf	PORTA,W

;
; test PORTA works as expected
;
	clrf	PORTA
	BANKSEL ANSELA
	clrf	ANSELA	; set port to digital
	BANKSEL TRISA
	movlw	0x38
	movwf	TRISA		;PORTA 0,1,2 output 3,4,5 input

	clrf	BSR	; bank 0
  .assert "porta == 0x00, \"PORTA = 0x00\""
	nop
	movlw	0x07
	movwf	PORTA		; drive 0,1,2  bits high
  .assert "porta == 0x3f, \"PORTA = 0x3f\""
	nop
	BANKSEL LATA
	movf	LATA,W
	BANKSEL TRISA
	movlw	0x07
	movwf	TRISA  	; PORTA 4, 5 output 0,1,2,3 input (3 input only)
	BANKSEL PORTA
  .assert "porta == 0x09, \"PORTA = 0x09\""
	nop
	movlw	0x38
	movwf	PORTA		; drive output bits high
  .assert "porta == 0x3f, \"PORTA = 0x3f\""
	nop

	call test_int
        call read_config_data
	call test_a2d
	call test_dac
 	call write_prog

  .assert  "\"*** PASSED 16f1503 Functionality\""
	nop
	reset
	goto	$



test_dac:
	BANKSEL ADCON1      
	movlw   0xf0 		;right justify, Frc, Vdd ref
	movwf   ADCON1		
	BANKSEL TRISC		;
	bsf     TRISC,0		;Set RC0 to input
	BANKSEL ANSELC		;
	bsf     ANSELC,0	;Set RC0 to analog
	BANKSEL ADCON0      	;
	movlw   (0x1e<<2)|(1<<ADON)	;Select DAC output and ADC on
	movwf   ADCON0      	
	; DAC output should be Vsource- (0)
	call	a2dConvert
   .assert "(adresh==0x00) && (adresl==0x00), \"*** FAILED 16f1503 DAC default\""
	nop
	; test DAC High idle state
	; Vdac = 5 * 31 / 32 = 4.844 V
	; A2D = (4.844 / 5) * 1023 = 0x3df
	BANKSEL DACCON0
	movlw   (1<<DACEN)	; Enable DAC with Vdd
	movwf	DACCON0
	movlw	0x1f		; top of ladder
	movwf	DACCON1
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x03) && (adresl==0xdf), \"*** FAILED 16f1503 DAC High power off\""
	nop
	; test DAC enabled Vout = 5V * (16/32) = 2.5
	BANKSEL ANSELA
	bsf	ANSELA,0	; porta0 analog input
	BANKSEL DACCON0
	movlw	0x10
	movwf	DACCON1
	movlw	(1<<DACEN)
	movwf	DACCON0
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x02) && (adresl==0x00), \"*** FAILED 16f1503 DAC enabled 1/2\""
	nop
  	; Test Vref+ = 2.5V Vout =2.5V * (16/32) = 1.25V
	; ADC = 1023 * 1.25V / 5.0V = 0x100
	BANKSEL	FVRCON
	movlw	(1<<FVREN)|(1<<CDAFVR1)|(1<<CDAFVR0)
	movwf	FVRCON
	movlw	(1<<DACEN)|(1<<DACPSS)
	movwf	DACCON0
	BANKSEL ADCON0
	call	a2dConvert
   .assert "(adresh==0x01) && (adresl==0x00), \"*** FAILED 16f1503 DAC Vref+=2.5V \""
	nop

	return


test_a2d
	banksel ANSELC
	clrf	ANSELC
	BANKSEL ADCON1      
	movlw   0x70 		;Left justify, Frc, Vdd ref
	movwf   ADCON1		
	BANKSEL TRISC		;
	bsf     TRISC,0		;Set RC0 to input
	BANKSEL ANSELC		;
	bsf     ANSELC,0	;Set RC0 to analog
	BANKSEL ADCON0      	;
	movlw   0x10|(1<<ADON)	;Select channel AN4 and ADC on
	movwf   ADCON0      	
	call	a2dConvert
   .assert "adresh == 0x33, \"*** FAILED 16f1503 AN4=1V\""
	nop
  ; measure Core Temperature
	bsf 	ADCON1,ADFM	; right justify result
        movlw	(0x1d << 2) | (1<<ADON) ; Core Temp channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x02) && (adresl==0x2f), \"*** FAILED 16f1503 ADC Core Temp\""
	nop

  ; measure FVR 
        movlw	(0x1f << 2) | (1<<ADON) ; FVR channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x01) && (adresl==0xa3), \"*** FAILED 16f1503 ADC FVR\""
	nop

  ; measure FVR using FVR Reference
	movlw	0xf3	; use FVR reference
	movwf	ADCON1
        movlw	(0x1f << 2) | (1<<ADON) ; FVR channel and AD on
	movwf	ADCON0
	call	a2dConvert
   .assert "(adresh==0x03) && (adresl==0xff), \"*** FAILED 16f1503 ADC FVR with FVR Ref\""
	nop

        clrf	ADRESH
        clrf    ADRESL
	movlw   0x20	; trigger a2d with t0
	movwf   ADCON2
        banksel OPTION_REG
        bcf     OPTION_REG,TMR0CS       ; tmr0 om fosc/4
        bcf	INTCON,TMR0IF
        btfss   INTCON,TMR0IF
	goto	$-1
        btfsc   ADCON0,GO
        goto    $-1
   .assert "(adresh==0x03) && (adresl==0xff), \"*** FAILED 16f1503 ADC FVR with FVR Ref\""
	nop
	clrf	ADCON2
        banksel OPTION_REG
        bsf     OPTION_REG,TMR0CS       ; tmr0 off fosc/4
	return

;
;	Start A2D conversion and wait for results
;
a2dConvert
	bsf 	ADCON0,GO
	btfsc	ADCON0,GO
	goto	$-1
	movf	ADRESH,W
	return


eefail:
  .assert "\"***FAILED 16f1503 eerom write/read error\""
	nop

test_tmr0:
	return
	





test_int:
	BANKSEL TRISA
	bsf     TRISA,2
        bcf     TRISA,5
	BANKSEL PORTA
	clrf	PORTA
	BANKSEL IOCAP
	bsf 	IOCAP,2		; set interrupt on + edge of porta2

	BANKSEL OPTION_REG
	bsf 	OPTION_REG,INTEDG
	BANKSEL IOCAF
	movlw	0xff
	movwf	IOCAF
   .assert "iocaf == 0x3f, \"*** FAILED 16f1503 INT test - IOCAF writable bits\""
	nop
	clrf	IOCAF		; Clear intcon IOCIF flag 
	movlw	0x7f
	movwf	INTCON
   .assert "intcon == 0x7e, \"*** FAILED 16f1503 INT test - INTCON:IOCIF read only\""
	nop
	clrf	INTCON
        bsf     INTCON,GIE      ;Global interrupts
        bcf     INTCON,PEIE     ;No Peripheral interrupts
        bsf     INTCON,IOCIE

	BANKSEL PORTA
        clrf    inte_cnt
        bsf     PORTA,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, \"*** FAILED 16f1503 INT test - No int on rising edge\""
        nop
   .assert "iocaf == 0x04, \"*** FAILED 16f1503 IOCAF bit 2 not set\""
	nop
	bcf	INTCON,IOCIF
   .assert "(intcon & 1) == 1, \"*** FAILED 16f1503 INTCON,IOCIF read only\""
	nop 
	BANKSEL	IOCAF
	bcf	IOCAF,2
   .assert "(intcon & 1) == 0, \"*** FAILED 16f1503 INTCON,IOCIF not cleared from IOCAF\""
	nop 
	clrf	inte_cnt
	bsf	INTCON,GIE	; Turn interrupts back on
	BANKSEL	PORTA
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "inte_cnt == 0x00, \"*** FAILED 16f1503 INT test - Unexpected int on falling edge\""
        nop


;	Setup - edge interrupt
	BANKSEL IOCAP
	clrf	IOCAP
	bsf 	IOCAN,2
	clrf	IOCAF
	BANKSEL INTCON

        clrf    inte_cnt
	clrf	iocaf_val
        bsf     PORTA,5          ; make a rising edge
        movf    inte_cnt,w
   .assert "W == 0x00, \"*** FAILED 16f1503 INT test - Unexpected int on rising edge\""
        nop
        clrf    inte_cnt
        bcf     PORTA,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, \"*** FAILED 16f1503 INT test - No int on falling edge\""
        nop
   .assert "iocaf == 0x04, \"*** FAILED 16f1503 IOCAF bit 2 not set\""
	nop
	BANKSEL	IOCAF
	CLRF	IOCAF
	bsf	INTCON,GIE
	BANKSEL	PORTA
        return

; read STKPTR, TOSL and cause underflow
rrr:
	nop
	BANKSEL STKPTR
	movf 	STKPTR,W
	movf	TOSL,W
  ;	clrf	TOSL
	call	rrr2
	return

; use STKPTR to cause stack underflow
rrr2:
	BANKSEL STKPTR
	movlw	0x1f
	movwf	STKPTR	;; cause stack underflow
	return

; overflow stack with recursive call
rrr3:
	call rrr3
	return
	

;
; Test reading and writing Configuration data via eeprom interface
read_config_data:
	;Read DeviceID at address 0x06 from config data
	BANKSEL  PMADRL            ; Select correct Bank
	movlw	0x06              ;
	movwf	PMADRL            ; Store LSB of address
	clrf	PMADRH            ; Clear MSB of address
	bsf 	PMCON1,CFGS       ; Select Configuration Space
	bcf 	INTCON,GIE        ; Disable interrupts
	bsf 	PMCON1,RD         ; Initiate read
	nop
	nop
	bsf 	INTCON,GIE        ; Restore interrupts
   .assert "pmdath == 0x2c && pmdatl == 0xe0, \"*** FAILED 16f1503 Device ID\""
	nop
	
  ; test write which is in PMDATAH and PMDATAL to userID2
	BANKSEL PIR2
	clrf	PIR2
	BANKSEL PMCON1
	movlw	0x01              ;
	movwf	PMADRL            ; Store LSB of address
	bsf 	PMCON1, WREN  	   ;Enable writes
	bcf 	INTCON,GIE	   ; disable interrupts
        movlw	0x55               ;Magic sequence to enable eeprom write
        movwf	PMCON2
        movlw	0xaa
        movwf	PMCON2
	bsf 	PMCON1,WR
	nop
	nop
	bsf  	INTCON,GIE
	bcf 	PMCON1, WREN  	;Disable writes
  .assert "UserID2 == 0x2ce0, \"*** FAILED 16f1503 write to UserID2\""
	nop
	return

clear_prog:
; This row erase routine assumes the following:
; 1. A valid address within the erase block is loaded in ADDRH:ADDRL
; 2. ADDRH and ADDRL are located in shared data memory 0x70 - 0x7F
	bcf       INTCON,GIE     ; Disable ints so required sequences will execute properly
        BANKSEL   PMADRL
 	movlw	0
        movwf   PMADRL
	movlw	3
        movwf   PMADRH
        bcf     PMCON1,CFGS    ;   Not configuration space
        bsf     PMCON1,FREE    ;   Specify an erase operation
        bsf     PMCON1,WREN    ;   Enable writes
        movlw   0x55           ;   Start of required sequence to initiate erase
        movwf   PMCON2
        movlw   0xAA           ;
        movwf   PMCON2
        bsf     PMCON1,WR      ;   Set WR bit to begin erase
        nop
        nop
        bcf     PMCON1,WREN    ; Disable writes
        bsf 	INTCON,GIE     ; Enable interrupts
	btfsc 	PMCON1,WR
        goto   $-2
	return

write_prog:

	bcf 	INTCON,GIE      ;   Disable ints so required sequences will execute properly
	BANKSEL	PMADRH          ;   Bank 3
	movlw	0x03
	movwf	PMADRH          ;
	movlw	0x00
	movwf	PMADRL          ;
	movlw	0x00
	movwf	FSR0L           ;
	movlw	0x20		;   Program memory
	movwf	FSR0H           ;
	bcf 	PMCON1,CFGS     ;   Not configuration space
	bsf 	PMCON1,WREN     ;   Enable writes
	bsf 	PMCON1,LWLO     ;   Only Load Write Latches
LOOP
	moviw	FSR0++          ; Load first data byte into lower
	movwf	PMDATL          ;
	moviw	FSR0++          ; Load second data byte into upper
	movwf	PMDATH          ;
	movf	PMADRL,W        ; Check if lower bits of address are '000'
	xorlw	0x07            ; Check if we're on the last of 8 addresses
	andlw	0x07            ;
	btfsc	STATUS,Z        ; Exit if last of eight words,
	goto	START_WRITE     ;
	movlw	0x55            ;   Start of required write sequence:
	movwf	PMCON2
	movlw	0xAA
	movwf	PMCON2
	bsf 	PMCON1,WR       ;   Set WR bit to begin write
	nop
	nop
	incf	PMADRL,F        ; Still loading latches Increment address
	goto	LOOP            ; Write next latches
START_WRITE
	bcf 	PMCON1,LWLO     ; No more loading latches - Actually start Flash program
	;	memory write
	movlw	0x55             ;   Start of required write sequence:
	movwf	PMCON2
	movlw	0xAA            ;
	movwf	PMCON2
	bsf 	PMCON1,WR       ;   Set WR bit to begin write
	nop
	nop
	bcf 	PMCON1,WREN     ; Disable writes
	bsf 	INTCON,GIE      ; Enable interrupts
	return

 	org 0x200
rrDATA
	dw 0x01, 0x02, 0x03
  end
