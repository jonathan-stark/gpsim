        list
; p16c65.inc  standard header file, version 1.00    microchip technology, inc.
        nolist

; this header file defines configurations, registers, and other useful bits of
; information for the pic16c65 microcontroller.  these names are taken to match 
; the data sheets as closely as possible.  

; note that the processor must be selected before this file is 
; included.  the processor may be selected the following ways:

;       1. command line switch:
;               c:\ mpasm myfile.asm /pic16c65
;       2. list directive in the source file
;               list   p=pic16c65
;       3. processor type entry in the mpasm full-screen interface

;==========================================================================
;
;       revision history
;
;==========================================================================

;rev:   date:    reason:

;1.00   10/31/95 initial release

;==========================================================================
;
;       verify processor
;
;==========================================================================

        ifndef __16c65
           messg "processor-header file mismatch.  verify selected processor."
        endif

;==========================================================================
;
;       register definitions
;
;==========================================================================

w                            equ     h'0000'
f                            equ     h'0001'

;----- register files------------------------------------------------------

indf                         equ     h'0000'
tmr0                         equ     h'0001'
pcl                          equ     h'0002'
status                       equ     h'0003'
fsr                          equ     h'0004'
porta                        equ     h'0005'
portb                        equ     h'0006'
portc                        equ     h'0007'
portd                        equ     h'0008'
porte                        equ     h'0009'
pclath                       equ     h'000a'
intcon                       equ     h'000b'
pir1                         equ     h'000c'
pir2                         equ     h'000d'
tmr1l                        equ     h'000e'
tmr1h                        equ     h'000f'
t1con                        equ     h'0010'
tmr2                         equ     h'0011'
t2con                        equ     h'0012'
sspbuf                       equ     h'0013'
sspcon                       equ     h'0014'
ccpr1l                       equ     h'0015'
ccpr1h                       equ     h'0016'
ccp1con                      equ     h'0017'
rcsta                        equ     h'0018'
txreg                        equ     h'0019'
rcreg                        equ     h'001a'
ccpr2l                       equ     h'001b'
ccpr2h                       equ     h'001c'
ccp2con                      equ     h'001d'

option_reg                   equ     h'0081'
trisa                        equ     h'0085'
trisb                        equ     h'0086'
trisc                        equ     h'0087'
trisd                        equ     h'0088'
trise                        equ     h'0089'
pie1                         equ     h'008c'
pie2                         equ     h'008d'
pcon                         equ     h'008e'
pr2                          equ     h'0092'
sspadd                       equ     h'0093'
sspstat                      equ     h'0094'
txsta                        equ     h'0098'
spbrg                        equ     h'0099'

;----- status bits --------------------------------------------------------

irp                          equ     h'0007'
rp1                          equ     h'0006'
rp0                          equ     h'0005'
not_to                       equ     h'0004'
not_pd                       equ     h'0003'
z                            equ     h'0002'
dc                           equ     h'0001'
c                            equ     h'0000'

;----- intcon bits --------------------------------------------------------

gie                          equ     h'0007'
peie                         equ     h'0006'
t0ie                         equ     h'0005'
inte                         equ     h'0004'
rbie                         equ     h'0003'
t0if                         equ     h'0002'
intf                         equ     h'0001'
rbif                         equ     h'0000'

;----- pir1 bits ----------------------------------------------------------

pspif                        equ     h'0007'
rcif                         equ     h'0005'
txif                         equ     h'0004'
sspif                        equ     h'0003'
ccp1if                       equ     h'0002'
tmr2if                       equ     h'0001'
tmr1if                       equ     h'0000'

;----- pir2 bits ----------------------------------------------------------

ccp2if                       equ     h'0000'

;----- t1con bits ---------------------------------------------------------

t1ckps1                      equ     h'0005'
t1ckps0                      equ     h'0004'
t1oscen                      equ     h'0003'
not_t1sync                   equ     h'0002'
t1insync                     equ     h'0002'    ; backward compatibility only
tmr1cs                       equ     h'0001'
tmr1on                       equ     h'0000'

;----- t2con bits ---------------------------------------------------------

toutps3                      equ     h'0006'
toutps2                      equ     h'0005'
toutps1                      equ     h'0004'
toutps0                      equ     h'0003'
tmr2on                       equ     h'0002'
t2ckps1                      equ     h'0001'
t2ckps0                      equ     h'0000'

;----- sspcon bits --------------------------------------------------------

wcol                         equ     h'0007'
sspov                        equ     h'0006'
sspen                        equ     h'0005'
ckp                          equ     h'0004'
sspm3                        equ     h'0003'
sspm2                        equ     h'0002'
sspm1                        equ     h'0001'
sspm0                        equ     h'0000'

;----- ccp1con bits -------------------------------------------------------

ccp1x                        equ     h'0005'
ccp1y                        equ     h'0004'
ccp1m3                       equ     h'0003'
ccp1m2                       equ     h'0002'
ccp1m1                       equ     h'0001'
ccp1m0                       equ     h'0000'

;----- rcsta bits ---------------------------------------------------------

spen                         equ     h'0007'
rx9                          equ     h'0006'
rc9                          equ     h'0006'    ; backward compatibility only
not_rc8                      equ     h'0006'    ; backward compatibility only
rc8_9                        equ     h'0006'    ; backward compatibility only
sren                         equ     h'0005'
cren                         equ     h'0004'
ferr                         equ     h'0002'
oerr                         equ     h'0001'
rx9d                         equ     h'0000'
rcd8                         equ     h'0000'    ; backward compatibility only

;----- ccp2con bits -------------------------------------------------------

ccp2x                        equ     h'0005'
ccp2y                        equ     h'0004'
ccp2m3                       equ     h'0003'
ccp2m2                       equ     h'0002'
ccp2m1                       equ     h'0001'
ccp2m0                       equ     h'0000'

;----- option bits --------------------------------------------------------

not_rbpu                     equ     h'0007'
intedg                       equ     h'0006'
t0cs                         equ     h'0005'
t0se                         equ     h'0004'
psa                          equ     h'0003'
ps2                          equ     h'0002'
ps1                          equ     h'0001'
ps0                          equ     h'0000'

;----- trise bits ---------------------------------------------------------

ibf                          equ     h'0007'
obf                          equ     h'0006'
ibov                         equ     h'0005'
pspmode                      equ     h'0004'
trise2                       equ     h'0002'
trise1                       equ     h'0001'
trise0                       equ     h'0000'

;----- pie1 bits ----------------------------------------------------------

pspie                        equ     h'0007'
rcie                         equ     h'0005'
txie                         equ     h'0004'
sspie                        equ     h'0003'
ccp1ie                       equ     h'0002'
tmr2ie                       equ     h'0001'
tmr1ie                       equ     h'0000'

;----- pie2 bits ----------------------------------------------------------

ccp2ie                       equ     h'0000'

;----- pcon bits ----------------------------------------------------------

not_por                      equ     h'0001'

;----- sspstat bits -------------------------------------------------------

d                            equ     h'0005'
i2c_data                     equ     h'0005'
not_a                        equ     h'0005'
not_address                  equ     h'0005'
d_a                          equ     h'0005'
data_address                 equ     h'0005'
p                            equ     h'0004'
i2c_stop                     equ     h'0004'
s                            equ     h'0003'
i2c_start                    equ     h'0003'
r                            equ     h'0002'
i2c_read                     equ     h'0002'
not_w                        equ     h'0002'
not_write                    equ     h'0002'
r_w                          equ     h'0002'
read_write                   equ     h'0002'
ua                           equ     h'0001'
bf                           equ     h'0000'

;----- txsta bits ---------------------------------------------------------

csrc                         equ     h'0007'
tx9                          equ     h'0006'
not_tx8                      equ     h'0006'    ; backward compatibility only
tx8_9                        equ     h'0006'    ; backward compatibility only
txen                         equ     h'0005'
sync                         equ     h'0004'
brgh                         equ     h'0002'
trmt                         equ     h'0001'
tx9d                         equ     h'0000'
txd8                         equ     h'0000'    ; backward compatibility only

;==========================================================================
;
;       ram definition
;
;==========================================================================

        __maxram h'ff'
        __badram h'1e'-h'1f',h'8f'-h'91', h'95'-h'97', h'9a'-h'9f'

;==========================================================================
;
;       configuration bits
;
;==========================================================================

_cp_all                      equ     h'3f8f'
_cp_75                       equ     h'3f9f'
_cp_50                       equ     h'3faf'
_cp_off                      equ     h'3fbf'
_pwrte_on                    equ     h'3fbf'
_pwrte_off                   equ     h'3fb7'
_wdt_on                      equ     h'3fbf'
_wdt_off                     equ     h'3fbb'
_lp_osc                      equ     h'3fbc'
_xt_osc                      equ     h'3fbd'
_hs_osc                      equ     h'3fbe'
_rc_osc                      equ     h'3fbf'

        list
