
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


        include "processor.inc"
        include <coff.inc>              ; Grab some useful macros

  __CONFIG	_CONFIG1H, _HSPLL_OSC_1H & _OSCS_ON_1H
  __CONFIG	_CONFIG2H, _WDT_ON_2H

  __CONFIG	_CONFIG3L, _LVP_OFF_4L


  __CONFIG	_CONFIG4L, _LVP_OFF_4L
  __CONFIG	_CONFIG5L, _CP0_ON_5L & _CP1_ON_5L
  __CONFIG	_CONFIG5H, _CPB_ON_5H

	include "ssd0323.inc"
	include "osram128x64.inc"
	include "portdef.inc"

;------------------------------------------------------------------------
; gpsim command
.command macro x
  .direct "C", x
  endm


;----------------------------------------------------------------------
;  Variable declarations
;----------------------------------------------------------------------
GPR_DATA                UDATA_ACS
temp            RES     1


temp0           RES     1
temp1           RES     1
temp2           RES     1
temp3           RES     1
  GLOBAL temp0, temp1, temp2, temp3


Tx RES 1
Ty RES 1

DISPLAY_DATA  UDATA 400
DisplayBuffer	RES LCD_nROWS*LCD_nCOLS/8
  GLOBAL DisplayBuffer

  EXTERN  CommandLoop
  EXTERN  SSD0323_Write

;------------------------------------------------------------------------
; Code labels

  GLOBAL done


;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        bra   start                      ; go to beginning of program

;----------------------------------------------------------------------
;   ******************* INTERRUPT VECTOR LOCATION  ******************
;----------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location
	RETFIE 1

;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

   ;;
   ;; Define the simulation environment.
   ;;

   .sim "module library libgpsim_graphicLCD"
   .sim "module library libgpsim_modules"

   .sim "module load OSRAM128X64 LCD"
   .sim "module load pullup R1"
   .sim "module load pullup R2"
   .sim "module load pullup R3"
   .sim "module load pullup R4"

   .sim "node nCS"
   .sim "node nRES"
   .sim "node nE"
   .sim "node nRW"
   .sim "node nDC"
   .sim "node nBS1"
   .sim "node nBS2"

   .sim "attach nE  porte0 LCD.e"
   .sim "attach nRW porte1 LCD.rw"
   .sim "attach nDC porte2 LCD.dc"

   .sim "attach nCS  portd0 LCD.cs R2.pin"
   .sim "attach nRES portd1 LCD.res"

   .sim "attach nBS2 R3.pin LCD.bs2"
   .sim "attach nBS1 R4.pin LCD.bs1"



   .sim "node nd0 nd1 nd2 nd3 nd4 nd5 nd6 nd7"

   .sim "attach nd0 portb0 LCD.d0"
   .sim "attach nd1 portb1 LCD.d1"
   .sim "attach nd2 portb2 LCD.d2"
   .sim "attach nd3 portb3 LCD.d3"
   .sim "attach nd4 portb4 LCD.d4"
   .sim "attach nd5 portb5 LCD.d5"
   .sim "attach nd6 portb6 LCD.d6"
   .sim "attach nd7 portb7 LCD.d7"

   ;; Now position the modules in the BreadBoard viewer

   .sim "LCD.xpos=220.0"
   .sim "LCD.ypos=24.0"
   .sim "R1.xpos=216.0"
   .sim "R1.ypos=264.0"
   .sim "R2.xpos=312.0"
   .sim "R2.ypos=264.0"
   .sim "R3.xpos=376.0"
   .sim "R3.ypos=264.0"

   ;; Initialize the SSD0323 LCD graphics controller.

	RCALL	SSD0323_Init

loop:
	CLRWDT

   ; Clear Screen Test

	RCALL	LCD_ClearScreen
	RCALL	LCD_RefreshDisplay

   ; Line Test

	movlw	63
	movwf	Ty
	movlw	127
	movwf	Tx
LL
	clrwdt
	movf	Tx,W
	MOVWF	LCD_x2
	movf	Ty,W
	MOVWF	LCD_y2
        MOVLW	0
	MOVWF	PixelX
	MOVLW	1
	MOVWF	PixelY

	RCALL	LCD_Line
;	RCALL	LCD_RefreshDisplay

	decf	Tx,F
	BNN	LL

	RCALL	LCD_RefreshDisplay

        MOVLW	0
	MOVWF	PixelX
	MOVWF	PixelY
	MOVLW	99
	MOVWF	LCD_x2
	MOVLW	15
	MOVWF	LCD_y2

	RCALL	LCD_Line
	RCALL	LCD_RefreshDisplay

	MOVLW	20
	MOVWF	PixelX
	CLRF	PixelY

	RCALL	LCD_ClearScreen

	MOVLW	1
	RCALL	LCD_putBitMap
	RCALL	LCD_RefreshDisplay

	RCALL	LCD_ClearScreen

	CLRF	PixelX
	MOVLW	0
	RCALL	LCD_putBitMap
	RCALL	LCD_RefreshDisplay



	MOVLW	32
	MOVWF	PixelX

	MOVLW	2
	RCALL	LCD_putBitMap
	RCALL	LCD_RefreshDisplay

	MOVLW	32
	MOVWF	PixelY
        CLRF    PixelX


	MOVLW	LOW(WindowCursor)
	MOVWF	TBLPTRL
	MOVLW	HIGH(WindowCursor)
	MOVWF	TBLPTRH

        MOVLW   WindowCursorEnd-WindowCursor
        MOVWF   temp0

        call    CommandLoop

        movlw   0
        movwf   Tx
        clrf    Ty
bars:
        
        rcall   SSD0323_Write
        incf    Ty,F
        bcf     Ty,4
        swapf   Ty,W
        iorwf   Ty,W

        decfsz  Tx,F
         bra bars


	bra	loop


WindowCursor:
        db 0x15, 0x08, 0x17,    0x75, 0x21, 0x30
WindowCursorEnd:


	nop

done:

  .assert  "\"*** PASSED LCD test\""
	goto	$

FAILED:
  .assert  "\"*** FAILED LCD test\""
	goto	$



  end
