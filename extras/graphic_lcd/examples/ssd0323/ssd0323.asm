;	
;  SSD0323 Driver for the PIC 18F family.
;
;  Copyright (C) 2007 T. Scott Dattalo
;
; This driver is designed to test gpsim. However, it is also usable 
; as a stand alone driver. You're free to use this driver as you
; wish as long as the copyright is retained. 
; 
;  
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
; The SSD0323 driver supports graphic LCD displays that are based on the
; Solomon Systech SSD0323 OLED graphics controller. The SSD0323 is an
; 128X80, 4-bit gray scale dot matrix controller.
;
;
; 
;  !!! NOTE: The driver assumes all variables reside in Bank 0!. !!!
;

        list    p=18f452,t=ON,c=132,n=80
        radix   dec



  #include <p18f452.inc>

; The defines in portdef.inc allow the LCD port definitions to be customized.

  #include "portdef.inc"
  #include "osram128x64.inc"

; Functions:

	GLOBAL	SSD0323_Init
	GLOBAL  SSD0323_WriteCmd
	GLOBAL  SSD0323_Write
	GLOBAL  LCD_RefreshDisplay	
        GLOBAL  CommandLoop

  EXTERN temp0, temp1, temp2, temp3
  EXTERN DisplayBuffer



;========================================================================
;
;   SSD0323 Driver
;
;
;   RAM Definitions

        UDATA_ACS
    ;--------------------
    ; Mode 
    ;--------------------

SSD0323_Mode	RES	1
        GLOBAL  SSD0323_Mode
; xxxx_xxxM
;         +--- Mode 0=Command 1=Data


;------------------------------------------------------------------------
; Macros

; SetMode 
;  Selects between Command and Data mode 
mSSD0323_SetMode  macro
	BCF	LATA, RA1
	BTFSC	SSD0323_Mode,bSSD0323_MODE_CMD
	 BSF	LATA, RA1
  endm



SETA0   macro
        BSF     LCDA0_LAT,LCDA0_BIT
  endm
CLRA0   macro
        BCF     LCDA0_LAT,LCDA0_BIT
  endm

;------------------------------------------------------------------------
; SSD0323 commands
;
; The SSD0323 Commands are written to address 1 of the chip.
; The data written for the command includes the command type and optional
; parameters. The data bus is only 8-bits wide. The upper bits in general
; describe the command while the lower are for data

SSD0323_DisplayOn               EQU     0xaf
SSD0323_DisplayOff              EQU     0xae
SSD0323_DisplayStartLine        EQU     0xc0
SSD0323_SETPAGEADDRESS          EQU     0xb8
SSD0323_SetColumnAddress        EQU     0x00
SSD0323_SetDuty                 EQU     0xa8

;------------------------------------------------------------------------
;


SSD0323_CODE	CODE


SSD0323_Init:

  ; Portb is the Data bus
  ; E - RE0
  ; RW - RE1
  ; DC - RE2
  ; RES - RD0
  ; CS - RD1

   ; Reset and CS are outputs
        BCF     LCDCS_TRIS,LCDCS_BIT
        BCF     LCDRES_TRIS,LCDRES_BIT

   ; Hold reset low while the ports are being initialized.
        BCF     LCDRES_LAT,LCDRES_BIT
        BSF     LCDCS_LAT,LCDCS_BIT

   ; Data bus
	CLRF	SSD0323_LAT
	CLRF	SSD0323_TRIS

   ; Control bus

  if LCDE_TRIS == LCDRW_TRIS && LCDE_TRIS == LCDDC_TRIS
	CLRF	TRISE
        SETF    LATE
  else
	BCF	LCDE_TRIS, LCDE_BIT
	BCF	LCDDC_TRIS, LCDDC_BIT
	BCF	LCDRW_TRIS, LCDRW_BIT
  endif

   ; Bring the chip out of reset:
        RCALL   delay
        BSF     LCDRES_LAT,LCDRES_BIT
        RCALL   delay

   ; Select the chip

        BCF     LCDCS_LAT,LCDCS_BIT



CmdSetColumnAddress     EQU 0x15
CmdGraphicAccleration   EQU 0x23
CmdDrawRectangle        EQU 0x24
CmdCopy                 EQU 0x25
CmdHorizontalScroll     EQU 0x26
CmdStopMoving           EQU 0x2E
CmdStartMoving          EQU 0x2F

CmdSetRowAddress        EQU 0x75
CmdSetContrast          EQU 0x81
CmdSetQuarterCurrent    EQU 0x84
CmdSetHalfCurrent       EQU 0x85
CmdSetFullCurrent       EQU 0x86

CmdSetRemap             EQU 0xA0
CmdSetDisplayStartLine  EQU 0xA1
CmdSetDisplayOffset     EQU 0xA2
CmdSetNormalDisplay     EQU 0xA4
CmdSetAllOn             EQU 0xA5
CmdSetAllOff            EQU 0xA6
CmdSetInverse           EQU 0xA7
CmdSetMultiplexRatio    EQU 0xA8
CmdSetMasterCfg         EQU 0xAD
CmdSetDisplayOff        EQU 0xAE
CmdSetDisplayOn         EQU 0xAF

CmdSetPreChargeCompensationEnable EQU 0xB0
CmdSetPhaseLength       EQU 0xB1
CmdSetRowPeriod         EQU 0xB2
CmdSetClockDivide       EQU 0xB3
CmdSetPreChargeCompensationLevel EQU 0xB4
CmdSetGrayScaleTable    EQU 0xB8

CmdSetPreChargeVoltage  EQU 0xBC
CmdSetVCOMH             EQU 0xBE
CmdSetVSL               EQU 0xBF
CmdNop                  EQU 0xE3

	MOVLW	LOW(CommandTable)
	MOVWF	TBLPTRL
	MOVLW	HIGH(CommandTable)
	MOVWF	TBLPTRH

        MOVLW   CommandTableEnd-CommandTable
        MOVWF   temp0

CommandLoop:
	TBLRD   *+
	MOVF	TABLAT,W
	rcall	SSD0323_WriteCmd
        decfsz  temp0,F
         bra    CommandLoop
        return

CommandTable:
        db CmdSetColumnAddress, 0x00, 0x3f,    CmdSetRowAddress, 0x00, 0x3f
        db CmdSetContrast, 0x6d
        db CmdSetFullCurrent,   CmdSetRemap, 0x41, CmdSetDisplayStartLine
        db 0x00, CmdSetDisplayOffset, 0x44, CmdSetNormalDisplay
        db CmdSetMultiplexRatio, 0x3f
        db CmdSetPhaseLength, 0x28
        db CmdSetPreChargeCompensationLevel, 0x07
        db CmdSetRowPeriod, 0x46
        db CmdSetClockDivide, 0x91
        db CmdSetVSL, 0x0d
        db CmdSetVCOMH, 0x02
        db CmdSetPreChargeVoltage, 0x10
        db CmdSetGrayScaleTable, 0x01,0x11,0x22, 0x32, 0x43,0x54,0x65 
        db 0x76, CmdSetMasterCfg, 02, CmdSetDisplayOn
CommandTableEnd:


CommandSetCursorPosition:
        db CmdSetColumnAddress, 0x00, 0x3f,    CmdSetRowAddress, 0x00, 0x3f
CommandSetCursorPositionEnd:

;------------------------------------------------------------
; Write either a command or data to the SSD0323
;
; Inputs: WREG - byte to be written
; Outputs:
; Mem used:  W
;

SSD0323_WriteCmd  ;<--- Entry point to write a command.
	BCF	LCDDC_LAT, LCDDC_BIT

SSD0323_Write:    ;<--- Entry point to write data.

	BCF	LCDRW_LAT, LCDRW_BIT    ;SSD0323 WR#

	MOVWF	SSD0323_LAT

	BSF	LCDRW_LAT, LCDRW_BIT    ;Rising edge writes the data
	BSF	LCDDC_LAT, LCDDC_BIT

	return

d2	rcall d3
d3	return

delay:  bra $+2
        bra $+2
        bra $+2
        decfsz WREG,F
         bra delay
        return
;------------------------------------------------------------------------
;LCD_RefreshDisplay - copy the RAM buffered display to the physical display
;
; Inputs:  None
; Outputs: None
; MemUsed: temp0

LCD_RefreshDisplay:	
	MOVLW	LOW(CommandSetCursorPosition)
	MOVWF	TBLPTRL
	MOVLW	HIGH(CommandSetCursorPosition)
	MOVWF	TBLPTRH

        MOVLW   CommandSetCursorPositionEnd-CommandSetCursorPosition
        MOVWF   temp0

        call    CommandLoop

	LFSR	0, DisplayBuffer

;	RCALL	SSD0323_Init

	MOVLW	0       ; row counter
	MOVWF	temp0

        MOVLW   1
        MOVWF   temp2   ; bit mask
SED_ref1:	


SED_ref2:
     ;row counter * 128
        MOVF    temp0,W
        MULLW   128

	LFSR	0, DisplayBuffer	;Get a pointer to the display
	MOVF	PRODL,W
	ADDWF	FSR0L,F	
	MOVF	PRODH,W
	ADDWFC	FSR0H,F

        MOVLW   128/2     ; # of columns
	MOVWF	temp1

SED_ref3:
        movf    temp2,W
        andwf   POSTINC0,W

        movlw   0
        skpz
         movlw  0x0f
        movwf   temp3

        movf    temp2,W
        andwf   POSTINC0,W

        movlw   0
        skpz
         movlw  0xf0
        iorwf   temp3,W

        
	rcall	SSD0323_Write

	DECFSZ	temp1,F
	 bra    SED_ref3

        rlncf   temp2,F
        btfss   temp2,0
         bra    SED_ref2
        incf    temp0,F
        btfss   temp0,3
         bra    SED_ref2

	return

;------------------------------------------------------------

  end
