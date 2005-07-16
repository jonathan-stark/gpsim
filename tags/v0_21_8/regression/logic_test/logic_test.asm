
   ;;  Logic test 
   ;;
   ;;  The purpose of this program is to verify that gpsim's
   ;; Logic modules function.
   ;;
   ;;
   ;;


	list    p=16f873                ; list directive to define processor
	include <p16f873.inc>           ; processor specific variable definitions
        include <coff.inc>              ; Grab some useful macros

        __CONFIG (_CP_OFF & _WDT_ON & _BODEN_ON & _PWRTE_ON & _HS_OSC & _WRT_ENABLE_ON & _LVP_OFF & _CPD_OFF)

        errorlevel -302 

;----------------------------------------------------------------------
;----------------------------------------------------------------------
GPR_DATA                UDATA
var1            RES     1
var2            RES     1
var3            RES     1
failures        RES     1

  GLOBAL var1,var2,var3,failures
  GLOBAL done
;----------------------------------------------------------------------
;   ********************* RESET VECTOR LOCATION  ********************
;----------------------------------------------------------------------
RESET_VECTOR  CODE    0x000              ; processor reset vector
        movlw  high  start               ; load upper byte of 'start' label
        movwf  PCLATH                    ; initialize PCLATH
        goto   start                     ; go to beginning of program


;----------------------------------------------------------------------
;   ******************* MAIN CODE START LOCATION  ******************
;----------------------------------------------------------------------
MAIN    CODE
start

        clrf    var1
        clrf    var2
        clrf    var3

        bsf     STATUS,RP0
        clrf    TRISA^0x80      ;Port A is an output
        clrf    TRISB^0x80      ;Port B is an output
        movlw   0xff
        movwf   TRISC^0x80      ;Port C is an input
        bcf     STATUS,RP0


  ; Create a script to define the schematic
  ; Note, the .sim directives are only "executed" once.

  ; First, instantiate the gates:

   .sim "module library libgpsim_modules.so"
   .sim "module load xor2 U1_xor"
   .sim "module load xor2 U2_xor"
   .sim "module load and2 U3_and"
   .sim "module load and2 U4_and"
   .sim "module load or2  U5_or"
   .sim "module load or2  U6_or"
   .sim "module load not  U7_not"
   .sim "module load not  U8_not"
   .sim "module load not  U9_not"

 
  ; Or Gate check. 
  ;            U5_or
  ; porta0  ---)-\     U6_or
  ;            )+ )----)-\
  ; porta1  ---)-/     )+ )---+
  ;                 +--)-/    |
  ; porta2  --------+         |
  ;                           |
  ; portc0  ------------------+
  ;



   .sim "node na0"
   .sim "attach na0 U5_or.in0 porta0"
   .sim "node na1"
   .sim "attach na1 U5_or.in1 porta1"
   .sim "node n5_6"
   .sim "attach n5_6 U5_or.out U6_or.in0"
   .sim "node na2"
   .sim "attach na2 U6_or.in1 porta2"
   .sim "node nc0"
   .sim "attach nc0 U6_or.out portc0"


	clrf	PORTA
	clrf	PORTB

   .assert "(portc&7) == 0"

	nop		;for the assert

   ; Loop through the 8 states:

L_OrCheck:
	incf	PORTA,F
   .assert "(portc & 1) == ( ((porta>>2) | (porta>>1) | porta) & 1)"
	btfss	PORTA,4
	 goto	L_OrCheck


  ; AND Gate check. 
  ;            U3_or
  ; porta3  ---|-+     U4_or
  ;            |& )----|-+
  ; porta4  ---|-+     |& )---+
  ;                 +--|-+    |
  ; porta5  --------+         |
  ;                           |
  ; portc1  ------------------+
  ;

   .sim "node na3"
   .sim "attach na3 U3_and.in0 porta3"
   .sim "node na4"
   .sim "attach na4 U3_and.in1 porta4"
   .sim "node n3_4"
   .sim "attach n3_4 U3_and.out U4_and.in0"
   .sim "node na5"
   .sim "attach na5 U4_and.in1 porta5"
   .sim "node nc1"
   .sim "attach nc1 U4_and.out portc1"

   ; Loop through the 8 states:	


L_AndCheck
	movlw	1<<3
	addwf	PORTA,F

   .assert "((portc>>1) & 1) == ( ((porta>>5) & (porta>>4) & (porta>>3)) & 1)"
	movf	PORTA,W
	andlw	b'111000'
	skpz	
	 goto	L_AndCheck



  ; xor Gate check. 
  ;            U1_xor
  ; portb0  ---))-\    U2_xor
  ;            ))^ )---))-\
  ; portb1  ---))-/    ))^ )--+
  ;                 +--))-/   |
  ; portb2  --------+         |
  ;                           |
  ; portc2  ------------------+
  ;



   .sim "node nb0"
   .sim "attach nb0 U1_xor.in0 portb0"
   .sim "node nb1"
   .sim "attach nb1 U1_xor.in1 portb1"
   .sim "node n1_2"
   .sim "attach n1_2 U1_xor.out U2_xor.in0"
   .sim "node nb2"
   .sim "attach nb2 U2_xor.in1 portb2"
   .sim "node nc2"
   .sim "attach nc2 U2_xor.out portc2"



   ; Loop through the 8 states:

L_XorCheck:
	incf	PORTB,F
   .assert "((portc>>2) & 1) == ( ((portb>>2) ^ (portb>>1) ^ portb) & 1)"
	btfss	PORTB,3
	 goto	L_XorCheck



  ; Not test

   .sim "node nb3"
   .sim "attach nb3 U7_not.in0 portb3"
   .sim "node nc3"
   .sim "attach nc3 U7_not.out portc3"

	bsf	PORTB, 3	;(don't need to do this...)
   .assert "((portc>>3) & 1) == ( ((portb>>3) & 1)^1)"
	bcf	PORTB, 3
   .assert "((portc>>3) & 1) == ( ((portb>>3) & 1)^1)"
	nop


   .sim "node nb4"
   .sim "attach nb4 U8_not.in0 portb4"
   .sim "node n7_8"
   .sim "attach n7_8 U8_not.out U9_not.in0"
   .sim "node nc4"
   .sim "attach nc4 U9_not.out portc4"


	bsf	PORTB, 4	;(don't need to do this...)
   .assert "((portc>>4) & 1) == ( ((portb>>4) & 1))"
	bcf	PORTB, 4
   .assert "((portc>>4) & 1) == ( ((portb>>4) & 1))"
	nop


done:
  ; If no expression is specified, then break unconditionally
  .assert  ""
        goto    done


        nop
  end


  end
