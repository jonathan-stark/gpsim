	; 12C509 gpsim regression test
	;
	; The purpose of this test is to verify that the 'C509
	; is implemented correctly. Here's a list of specific things
	; tested:	
	;
	; 1) Reset conditions
	; 2) WDT
	; 3) Sleep
	; 4) Wakeup on PIN change.

	list	p=12c509
        radix   dec

	nolist
include "p12c509.inc"

	list



  __CONFIG _WDT_ON

  cblock  0x07

	ResetSequence
  endc


    ; Define the reset conditions to be checked.

eRSTSequence_PowerOnReset	equ	1
eRSTSequence_AwakeMCLR		equ	2
eRSTSequence_AwakeWDT		equ	3
eRSTSequence_AwakeIO		equ	4
eRSTSequence_WDTTimeOut		equ	5

	org 0

;************************************************************************
;
START:

        MOVWF   OSCCAL          ; put calibration into oscillator cal reg


   ; OPTION register setup
   ;
   ;  NOT_GPWU = 0  - Enable wakeup if I/O pins 0,1, or 3 change states
   ;  NOT_GPPU = 1  - Disable weak pullups on I/O pins 0,1, and 3 
   ;  TOCS = 0 - Let the clock source for TMR0 be the internal fosc/4
   ;  TOSE = 0 - don't care - TMR0 source edge.
   ;  PSA = 1 - assign Prescale to the WDT
   ;  PS2:0 = 111 - prescale = 128

	MOVLW	(1<<NOT_GPPU) | (1<<PSA) | (1<<PS2) | (1<<PS1) | (1<<PS0)
        OPTION

   ; GPIO setup
   ; If this is a PowerOn reset, then the state of the GPIO output register
   ; is unknown. If this is not a PowerOn reset then the GPIO pins have
   ; the same state they had prior to the reset.

   ;
   ; TRIS register setup - The I/O pins are always configured as Inputs after
   ; any reset.

        MOVLW   1<<bSWITCH      ;Only the switch is an input
        TRIS    GPIO            ;

   ; Now examine why the RESET condition occurred.

	BTFSC	STATUS,GPWUF	;Did we just wake up from SLEEP due
         goto   AwakeIO		;to a change in the I/O pins?

	BTFSS	STATUS,NOT_TO
	 goto   TO_is_low

	BTFSS	STATUS,NOT_PD
	 goto   AwakeMCLR	;/TO=1, /PD=0 ==> /MCLR while sleeping

	goto	PowerOnReset	;/TO=1, /PD=1 ==> Power has just been applied

TO_is_low

	BTFSS	STATUS,NOT_PD
	 goto   AwakeWDT	;/TO=0, /PD=0 ==> WDT while sleeping

	goto	WDTTimeOut	;/TO=0, /PD=1 ==> WDT while not sleeping


;========================================================================
;
;  AwakeIO - an I/O pin changed states to awaken us from sleep.
;
;========================================================================
AwakeIO:
	MOVLW	eRSTSequence_AwakeIO
	MOVWF	ResetSequence

	CLRWDT
	SLEEP

;========================================================================
;
;  PowerOnReset
;
;========================================================================
PowerOnReset:

	MOVLW	eRSTSequence_PowerOnReset
	MOVWF	ResetSequence

	CLRWDT
	SLEEP

;========================================================================
;
;  AwakeWDT - WDT timed out while we were sleeping.
;
;========================================================================
AwakeWDT:

	MOVLW	eRSTSequence_AwakeWDT
	MOVWF	ResetSequence

	CLRWDT
	SLEEP

;========================================================================
;
;  AwakeMCLR - MCLR went low while we were sleeping.
;
;========================================================================
AwakeMCLR:

	MOVLW	eRSTSequence_AwakeMCLR
	MOVWF	ResetSequence

	CLRWDT
	SLEEP

;========================================================================
;
;  WDTTimeOut - WatchDog timer timed out while we were awake.
;
;========================================================================
WDTTimeOut:

	MOVLW	eRSTSequence_WDTTimeOut
	MOVWF	ResetSequence

	CLRWDT
	SLEEP


done:
	goto	done

failure:
	INCF	failures,F
	goto	done

	ORG	0x3ff
OSC_CAL_VALUE	EQU	0x80

	MOVLW	OSC_CAL_VALUE
	
	end
