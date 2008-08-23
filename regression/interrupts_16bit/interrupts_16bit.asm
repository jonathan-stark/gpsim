
        ;; The purpose of this program is to test gpsim's ability to 
        ;; simulate interrupts on the midrange core (specifically the 'f84).


	list    p=18f2321               ; list directive to define processor
	include <p18f2321.inc>          ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

 ;; Suppress warnings of using 'option' instruction.
	errorlevel -224

 CONFIG WDT=OFF
 CONFIG MCLRE=ON, LPT1OSC=OFF, PBADEN=DIG, CCP2MX=RC1



GPR_DATA        UDATA
temp            RES     1
temp1           RES     1
temp2           RES     1
temp3           RES     1
temp4           RES     1
temp5           RES     1
adr_cnt         RES     1
data_cnt        RES     1
failures        RES     1

w_temp          RES     1
status_temp     RES     1

 GLOBAL temp,temp1,temp2,temp3,temp4,temp5

 GLOBAL start
;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program




;------------------------------------------------------------------------
;
;  Interrupt Vector
;
;------------------------------------------------------------------------

INT_VECTOR   CODE    0x008               ; interrupt vector location

        movwf   w_temp
        swapf   STATUS,W
        movwf   status_temp

check_rbi:
        btfsc   INTCON,RBIF
         btfss  INTCON,RBIE
          bra   check_int

        bsf     temp5,1         ;Set a flag to indicate rb4-7 int occured
        bcf     INTCON,RBIF
	movf	PORTB,w
        
check_int:
        btfsc   INTCON,INT0IF
         btfss  INTCON,INT0IE
          bra   check_t0

        bsf     temp5,0         ;Set a flag to indicate rb0 int occured
        bcf     INTCON,INT0IF

check_t0:
        btfsc   INTCON,T0IF
         btfss  INTCON,T0IE
          bra   exit_int

    ;; tmr0 has rolled over
        
        bcf     INTCON,T0IF     ; Clear the pending interrupt
        bsf     temp1,0         ; Set a flag to indicate rollover
                
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
        ;; Assume no failures

        clrf    failures

        movlw   0Fh    ; Configure all A/D
        movwf   ADCON1 ; for digital inputs
        movwf   07h    ; Configure comparators
        movwf   CMCON  ; for digital input

        ;;
        ;; tmr0 test
        ;;
        ;; The following block of code tests tmr0 together with interrupts.
        ;; Each prescale value (0-7) is loaded into the timer. The software
        ;; waits until the interrupt due to tmr0 rollover occurs before
        ;; loading the new prescale value.
        
        bsf     INTCON,T0IE     ;Enable TMR0 overflow interrupts
        
        bsf     INTCON,GIE      ;Global interrupts

        clrf    temp1           ;Software flag used to monitor when the
                                ;interrupt has been serviced.

        movlw   0xC0
        movwf   T0CON           ;Assign new prescale value

test_tmr0:      
        btfss   temp1,0         ;Wait for the interrupt to occur and
         bra    test_tmr0       ;get serviced

        
        clrf    temp1           ;Clear flag for the next time
        
        incf    T0CON,f
        
        btfss   T0CON,3         ; When we've done all prescaler values, PSA gets set
         bra    test_tmr0

        bcf     INTCON,T0IE     ;Disable tmr0 interrupts

        ;; Now check tmr0 with an external clock source
        ;;
        ;; It assumes that port b bit 0 is the stimulus.
        ;; This requires that the following gpsim commands be invoked:
        ;;  gpsim> node new_test_node
        ;;  gpsim> attach new_test_node porta4 portb0

        bcf     TRISB,0        ;portb bit 0 is an output

        ;; assign the prescaler to the wdt so that tmr0 counts every edge
        ;; select the clock source to be tocki
        ;; and capture low to high transitions (tose = 0)
        
        movlw   (1<<T0CS) | (1<<PSA) | (1<<TMR0ON) | (1<<T08BIT)
        movwf   T0CON

        movlw   0xff
        movwf   temp2
        movwf   temp3
        movwf   temp4
        
 if 1   ; This is really a test of timer 0 functionality, not interrupts

        bcf     INTCON,T0IF     ;not necessary..., but clear pending int.
        bsf     INTCON,T0IE     ;Re-enable tmr0 interrupts

tmr0_l1:
        bcf     temp1,0         ;Interrupt flag

        clrz    
        bcf     PORTB,0
        btfsc   temp1,0
         decf   temp2,f         ;Falling edge caused the interrupt

        bcf     temp1,0
        bsf     PORTB,0
        btfsc   temp1,0
         decf   temp3,f         ;Rising edge caused the interrupt

        ;; if either temp2 or temp3 decremented to zero, then z will be set
        skpz
         bra    tmr0_l1

        incfsz  temp4,f
         bra    test_inte
        
        ;; Now let's test external clocking with the falling edge
        
        movlw   (1<<T0CS) | (1<<PSA) | (1<<T0SE) | (1<<TMR0ON) | (1<<T08BIT)
        movwf   T0CON
        
        bra     tmr0_l1

        ;; tmr0 test is done.
 endif
        ;;
        ;; inte test
        ;;
        ;; The following block of code tests the interrupt on
        ;; change for port b bit 0. It assumes that port a bit 4
        ;; is the stimulus. This requires that the following
        ;; gpsim commands be invoked:
        ;;  gpsim> node new_test_node
        ;;  gpsim> attach new_test_node porta4 portb0
        ;;
        ;; Also, recall that porta bit 4 is an open collector
        ;; output (it can't drive high). So to generate the
        ;; the logic highs, the portb weak pull-up resistors
        ;; need to be enabled.
        
test_inte:

        
        bsf     TRISB,0        ;and portb bit 0 an input
        bcf     TRISA,4        ;Make porta bit 4 an output
        bcf     INTCON2,NOT_RBPU  ;Enable the portb weak pull-ups
        bsf     INTCON2,INTEDG0   ;Interrupt on rising edge


        movlw   0xff
        movwf   temp1
        movwf   temp2
        movwf   temp3
        movwf   temp4
                
        bcf     PORTA,4
        bcf     INTCON,INT0IF
        bsf     INTCON,INT0IE


        ;;
        ;; This routine toggles porta bit 4 with the hope of generating
        ;; an interrupt on portb bit 0.
        ;; temp1 counts the number of times an interrupt has occurred
        ;; temp2 counts the number of times the interrupt was due to
        ;;      a rising edge.
inte_edgecount:
        bcf     temp5,0         ;Interrupt flag

        clrz    
        bcf     PORTA,4         ;Falling edge

        btfsc   temp5,0
         decf   temp2,f         ;Falling edge caused the interrupt

        bcf     temp5,0
        bsf     PORTA,4         ;Rising edge

        btfsc   temp5,0
         decf   temp3,f         ;Rising edge caused the interrupt

        ;; if either temp2 or temp3 decremented to zero, then z will be set
        bnz     inte_edgecount

        incfsz  temp4,f
         bra    test_rbif
        
        ;; Now let's test the falling edge
        
        bcf     INTCON,INT0IE             ;Disable inte interrupt
        bcf     PORTA,4
        bcf     INTCON,INT0IF
        
        bcf     INTCON2,INTEDG0    ;Interrupt on falling edge

        bsf     INTCON,INT0IE
        
        goto    inte_edgecount

        ;;
        ;; test_rbif
        ;;
        ;; This next block tests the interrupt on change feature of
        ;; port b's I/O pins 4-7
        ;; 
test_rbif

        bcf     INTCON,INT0IE     ;Disable the rb0 interrupt

        bsf     TRISA,4        ;Porta bit 4 is now an input 

        clrf    temp5                   ;Interrupt flag
        clrf    temp1
        
        movlw   0x10
        movwf   temp2

	movlw	6
	movwf	PORTB

rbif_l1:
        
        bcf     INTCON,RBIE
        
        movf    temp2,w
        movwf   TRISB

;        clrf    PORTB
        movf    PORTB,W

        clrf    temp5                   ;Interrupt flag

        bcf     INTCON,RBIF
        bsf     INTCON,RBIE


        swapf   temp2,w
        xorwf   PORTB,f

        btfsc   temp5,1
         iorwf  temp1,f

   .assert "(temp5 & 2) == 2"

        clrc
        rlcf    temp2,f

        skpc
         bra    rbif_l1


done:   
  .assert  "\"*** PASSED 16bit-core basic interrupt test\""
        goto    $

failed:	
  .assert  "\"*** FAILED 16bit-core basic interrupt test\""
        goto    $

        end
