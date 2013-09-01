
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


	list    p=12f629                ; list directive to define processor
	include <p12f629.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG _CP_OFF & _WDT_ON &  _INTRC_OSC_NOCLKOUT & _PWRTE_ON &  _BODEN_OFF & _MCLRE_OFF


;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA_SHR
temp            RES     1

w_temp          RES     1
status_temp     RES     1
cmif_cnt	RES	1
tmr0_cnt	RES	1
tmr1_cnt	RES	1
eerom_cnt	RES	1
adr_cnt		RES	1
data_cnt	RES	1
inte_cnt	RES	1



;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


  .sim "node n1"
  .sim "attach n1 gpio0 gpio3"
  .sim "node n2"
  .sim "attach n2 gpio1 gpio4"
  .sim "node n3"
  .sim "attach n3 gpio2 gpio5"


;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------
                                                                                
INT_VECTOR   CODE    0x004               ; interrupt vector location
                                                                                
        movwf   w_temp
        swapf   STATUS,W
	bcf	STATUS,RP0	; set bank 0
        movwf   status_temp

	btfsc	PIR1,EEIF
	    goto ee_int

  	btfsc	INTCON,T0IF
	    goto tmr0_int

  	btfsc	INTCON,INTF
	    goto inte_int

	.assert "\"***FAILED p12f629 unexpected interrupt\""
	nop


; Interrupt from TMR0
tmr0_int
	incf	tmr0_cnt,F
	bcf	INTCON,T0IF
	goto	exit_int

; Interrupt from eerom
ee_int
	incf	eerom_cnt,F
	bcf	PIR1,EEIF
	goto	exit_int

; Interrupt from INT pin
inte_int
	incf	inte_cnt,F
	bcf	INTCON,INTF
	goto	exit_int

exit_int:
                                                                                
        swapf   status_temp,w
        movwf   STATUS
        swapf   w_temp,f
        swapf   w_temp,w
        retfie
                                                                                

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start
	;
	; test pins in analog mode return 0 on register read
	BANKSEL TRISIO
	clrf	TRISIO
	BANKSEL GPIO
	movlw	0xff
	movwf	GPIO
   .assert "gpio == 0x3C, \"**FAILED 12f629  analog bits read 0\""
	nop
	movf	GPIO,W

	movlw	0xff
	movwf	CMCON		; turn off Comparator
    .assert "cmcon == 0x1f, \"**FAILED 12f629 CMCON read 0 bits\""
	nop
;
; test GPIO works as expected
;
	clrf	GPIO
	bsf	STATUS,RP0
	movlw	0x38
	movwf	TRISIO		;GPIO 0,1,2 output 3,4,5 input
	bcf	STATUS,RP0
  .assert "gpio == 0x00, \"GPIO = 0x00\""
	nop
	movlw	0x07
	movwf	GPIO		; drive 0,1,2  bits high
  .assert "gpio == 0x3f, \"GPIO = 0x3f\""
	nop
	bsf     STATUS,RP0
	movlw	0x07
	movwf	TRISIO  	; GPIO 4, 5 output 0,1,2,3 input (3 input only)
        bcf     STATUS,RP0
  .assert "gpio == 0x09, \"GPIO = 0x09\""
	nop
	movlw	0x38
	movwf	GPIO		; drive output bits high
  .assert "gpio == 0x3f, \"GPIO = 0x3f\""
	nop

	call test_eerom
	call test_int

  .assert  "\"*** PASSED 12f629 Functionality\""
	goto	$






test_eerom:
  ;
  ;	test can write and read to all 128 eeprom locations
  ;	using intterupts
        clrf    adr_cnt
        clrf    data_cnt
;  setup interrupts
        bsf     INTCON,PEIE
        bsf     INTCON,GIE
	BANKSEL PIE1
	bsf	PIE1,EEIE
	BANKSEL	PIR1
;
;	write to EEPROM starting at EEPROM address 0
;	value of address as data using interrupts to
;	determine write complete. 
;	read and verify data

l1:     
        movf    adr_cnt,W
	clrf	eerom_cnt
	BANKSEL	EEADR
        movwf   EEADR 
        movf    data_cnt,W
        movwf   EEDATA

        bcf     INTCON,GIE      ;Disable interrupts while enabling write

        bsf     EECON1,WREN    ;Enable eeprom writes

        movlw   0x55            ;Magic sequence to enable eeprom write
        movwf   EECON2
        movlw   0xaa
        movwf   EECON2

        bsf     EECON1,WR      ;Begin eeprom write

        bsf     INTCON,GIE      ;Re-enable interrupts
        
	BANKSEL	PIR1
        clrf    STATUS          ; Bank 0
        movf   eerom_cnt,W
	skpnz
        goto   $-2
;
;	read what we just wrote
;
	
        movf    adr_cnt,W

	BANKSEL	EEADR
	movwf   EEADR
	bsf	EECON1,RD	; start read operation
	movf	EEDATA,W	; Read data
	BANKSEL	PIR1

	xorwf	data_cnt,W	; did we read what we wrote ?
	skpz
	goto eefail

        incf    adr_cnt,W
        andlw   0x7f
        movwf   adr_cnt
	movwf	data_cnt

        skpz
         goto   l1

	return

eefail:
  .assert "\"***FAILED 12f629 eerom write/read error\""
	nop

test_tmr0:
	return
	





test_int:
	BANKSEL TRISIO
	bsf     TRISIO,2
        bcf     TRISIO,5
	BANKSEL GPIO
	clrf	GPIO

	BANKSEL OPTION_REG
	bsf	OPTION_REG,INTEDG
	BANKSEL INTCON
        bsf     INTCON,GIE      ;Global interrupts
        bcf     INTCON,PEIE     ;No Peripheral interrupts
        bcf     INTCON,INTF     ;Clear flag
        bsf     INTCON,INTE

        clrf    inte_cnt
        bsf     GPIO,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, \"*** FAILED 12f629 INT test - No int on rising edge\""
        nop
        clrf    inte_cnt
        bcf     GPIO,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x00, \"*** FAILED 12f629 INT test - Unexpected int on falling edge\""
        nop


	BANKSEL OPTION_REG
	bcf	OPTION_REG,INTEDG
	BANKSEL INTCON
        bcf     INTCON,INTF     ;Clear flag

        clrf    inte_cnt
        bsf     GPIO,5          ; make a rising edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x00, \"*** FAILED 12f629 INT test - Unexpected int on rising edge\""
        nop
        clrf    inte_cnt
        bcf     GPIO,5          ; make a falling edge
        nop
        movf    inte_cnt,w
   .assert "W == 0x01, \"*** FAILED 12f629 INT test - No int on falling edge\""
        nop


        return

	
  end
