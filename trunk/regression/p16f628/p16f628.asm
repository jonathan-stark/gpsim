
   ;;  16F628 tests
   ;;
   ;; This regression test exercises the 16f628.  
   ;; 
   ;;  Tests performed:
   ;;
   ;;  1) Verify Comparators can be disabled and that
   ;;     PORTA can work as a digital I/O port.


   list p=16f628

include "p16f628.inc"


   cblock 0x20

	failures

   endc


	ORG	0

	CLRF	failures	;Assume success

	MOVLW	0xff
	BSF	STATUS,RP0

	CLRF	TRISA^0x80	;Port B is an output
	MOVWF	TRISB^0x80	;Port C is an input

	CLRF	VRCON^0x80	;Turn off the Voltage Reference module.

	BCF 	STATUS,RP0

   ;;
   ;; Turn off the comparator module
   ;;
 
	MOVLW	(1<<CM0) | (1<<CM1) | (1<<CM2)
	MOVWF	CMCON

	MOVLW	0x0F

a_to_b_loop:
	MOVWF	PORTA		;Port A and Port B are externally
	XORWF	PORTB,W		;connected. So we should see the
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTB,W
	 goto	a_to_b_loop

	GOTO	done
FAILED:
	INCF	failures,F
done:

	GOTO	$


  end
