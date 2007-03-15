

;------------------------------------------------------------------------
;
; I/O Port definitions for the SSD0323 based graphic LCD
;
;
; The user of the graphic LCD module should configure this
; file according to their specific application.
;

LCDCS_PORT   EQU   PORTD
LCDCS_TRIS   EQU   TRISD
LCDCS_LAT    EQU   LATD
LCDCS_BIT    EQU   RD0

LCDRES_PORT  EQU   PORTD
LCDRES_TRIS  EQU   TRISD
LCDRES_LAT   EQU   LATD
LCDRES_BIT   EQU   RD1

LCDE_PORT    EQU   PORTE
LCDE_TRIS    EQU   TRISE
LCDE_LAT     EQU   LATE
LCDE_BIT     EQU   RE0

LCDRW_PORT   EQU   PORTE
LCDRW_TRIS   EQU   TRISE
LCDRW_LAT    EQU   LATE
LCDRW_BIT    EQU   RE1

LCDDC_PORT   EQU   PORTE
LCDDC_TRIS   EQU   TRISE
LCDDC_LAT    EQU   LATE
LCDDC_BIT    EQU   RE2


SSD0323_PORT EQU   PORTB
SSD0323_TRIS EQU   TRISB
SSD0323_LAT  EQU   LATB


