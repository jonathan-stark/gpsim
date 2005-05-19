        list    p=16c84
        radix dec
        ;; The purpose of this program is to test gpsim macros
        ;;

include "p16c84.inc"

  cblock  0x0c
        failures
        mac_count
	mac_flags
  endc

        org     0
        goto    start
        
        org     4
start:
        clrf    failures
        clrf    mac_count           ; a counter
	clrf	mac_flags

mac_loop:
	incf	mac_count,F
	btfss	mac_flags,4
	 goto	mac_loop



done:
        goto    done

        end
