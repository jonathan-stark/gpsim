
   ;;  16F84 tests
   ;;
   ;; This regression test exercises the 16f84.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  - Verify that the PORTB internal pull-ups work
   ;;     


   list p=16f84

include "p16f84.inc"

CONFIG_WORD	EQU	_CP_OFF & _WDT_OFF

        __CONFIG  CONFIG_WORD

   cblock 0x20

	failures

   endc


	ORG	0

	CLRF	failures	;Assume success

	MOVLW	0xff
	BSF	STATUS,RP0

	CLRF	TRISA^0x80	;Port A is an output
	MOVWF	TRISB^0x80	;Port B is an input

	BCF 	STATUS,RP0


	MOVLW	0x0F

a_to_b_loop:
	MOVWF	PORTA		;Port A and Port B are externally
	XORWF	PORTB,W		;connected. So we should see the
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTB,W
	 goto	a_to_b_loop

	BSF	PORTA,4		;Port A bit 4 is an open collector.
	BTFSC	PORTB,4
	 GOTO	FAILED


   ; Now let's write from PORTB to PORTA.
   ; With the configuration bit setting we have, all of PORTA I/O lines
   ; should be able to serve as inputs

	BSF	STATUS,RP0

	COMF	TRISA^0x80,F	;Port A is now an input port
	COMF	TRISB^0x80,F	;Port B is now an output port

	BCF 	STATUS,RP0

	CLRW


b_to_a_loop:
	MOVWF	PORTB		;Port A and Port B are externally
	XORWF	PORTA,W		;connected. So we should see the
	ANDLW	0x1f
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTB,W
	 goto	b_to_a_loop

   ;
   ; Now test PORTB's internal pullups
   ;

	CLRF	PORTB

	MOVLW	0xff
	BSF	STATUS,RP0

	MOVWF	TRISA^0x80	;Port A is an input
	MOVWF	TRISB^0x80	;Port B is an input

	BSF	OPTION_REG ^ 0x80, NOT_RBPU	;Disable the pullups

	BCF 	STATUS,RP0

	MOVF	PORTB,W
	ANDLW	0x0F
	SKPZ
	 GOTO	FAILED

	BSF	STATUS,RP0
	BCF	OPTION_REG ^ 0x80, NOT_RBPU	;Enable the pullups
	BCF 	STATUS,RP0

	MOVF	PORTB,W		;All lines should be high
	XORLW	0xFF
	SKPZ
	 GOTO	FAILED

	GOTO	done

FAILED:
	INCF	failures,F
done:
	GOTO	$


  end
