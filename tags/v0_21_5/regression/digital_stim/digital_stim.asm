        list    p=16c84
        radix dec
        ;; The purpose of this program is to test gpsim's stimulation capability
        ;;

include "p16c84.inc"

  cblock  0x20

        failures

        temp1
        temp2
        temp3
  endc

        org     0
        goto    start
        
        org     4
start:
        clrf    failures
        clrf    temp1           ; a counter
        clrf    temp2           ; a flag keeping track of the state of port b
begin:

        ;; Count the rising edges on portb bit 0
        
        movf    PORTB,W
        xorwf   temp2,w
        xorwf   temp2,f
        andwf   temp2,w
        andlw   1
        
        skpz
          incf  temp1,f
        
        btfss   PORTB,1
         goto   begin

        movf    temp1,W

#define EXPECTED_PULSES .20

        xorlw   EXPECTED_PULSES
        skpz
         incf   failures,F
done:
        goto    done

        end
