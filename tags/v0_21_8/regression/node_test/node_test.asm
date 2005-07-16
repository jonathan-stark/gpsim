
   ;;  Node test 
   ;;
   ;;  The purpose of this program is to verify that nodes
   ;; can interconnect I/O pins.


   list p=16f877

include "p16f877.inc"


   cblock 0x20

	failures

   endc


	ORG	0

	CLRF	failures	;Assume success

	MOVLW	0xff
	BSF	STATUS,RP0

	CLRF	TRISB^0x80	;Port B is an output
	MOVWF	TRISC^0x80	;Port C is an input

	BCF 	STATUS,RP0

b_to_c_loop:
	MOVWF	PORTB		;Port B and Port C are externally
	XORWF	PORTC,W		;connected. So we should see the
	SKPZ			;same thing on each port.
	 GOTO	FAILED

	DECFSZ	PORTC,W
	 goto	b_to_c_loop

	GOTO	done

FAILED:
	INCF	failures,F
done:

	GOTO	$


  end
