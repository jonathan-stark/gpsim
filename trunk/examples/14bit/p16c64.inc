
;;
;;	Include file for the p16c64
;;

;----- register files------------------------------------------------------

indf		equ	0x0
tmr0		equ	0x1
pcl		equ	0x2
status		equ	0x3
fsr		equ	0x4
porta		equ	0x5
portb		equ	0x6
portc		equ	0x7
portd		equ	0x8
porte		equ	0x9

pclath		equ	0xa
intcon		equ	0xb
pir1		equ     0xc

tmr1l		equ     0xe
tmr1h		equ     0xf
t1con		equ     0x10
tmr2		equ     0x11
t2con		equ     0x12
sspbuf		equ     0x13
sspcon		equ     0x14
ccpr1l		equ     0x15
ccpr1h		equ     0x16
ccp1con		equ     0x17


option_reg	equ	0x81
trisa		equ	0x85
trisb		equ	0x86
trisc		equ	0x87
trisd		equ	0x88
trise		equ	0x89

pie1		equ	0x8c
pcon		equ	0x8e

pr2		equ	0x92
sspadd		equ	0x93
sspstat		equ	0x94


;----- status bits --------------------------------------------------------

irp		equ	7
rp1		equ	6
rp0		equ	5
not_to		equ	4
to		equ	4
not_pd		equ	3
pd		equ	3
z		equ	2
dc		equ	1
c		equ	0

;----- intcon bits --------------------------------------------------------

gie		equ	7
peie		equ	6
t0ie		equ	5
inte		equ	4
rbie		equ	3
t0if		equ	2
intf		equ	1
rbif		equ	0

;----- PIR1 Bits ----------------------------------------------------------

pspif		equ	7
sspif		equ	3
ccp1if		equ	2
tmr2if		equ	1
tmr1if		equ	0

;----- t1con bits ---------------------------------------------------------

t1ckps1		equ	5
t1ckps0		equ	4
t1oscen		equ	3
not_t1sync	equ	2
t1insync	equ	2
tmr1cs		equ	1
tmr1on		equ	0

;----- t2con bits ---------------------------------------------------------

toutps3		equ	6
toutps2		equ	5
toutps1		equ	4
toutps0		equ	3
tmr2on		equ	2
t2ckps1		equ	1
t2ckps0		equ	0

;----- sspcon bits --------------------------------------------------------

wcol		equ	7
sspov		equ	6
sspen		equ	5
ckp		equ	4
sspm3		equ	3
sspm2		equ	2
sspm1		equ	1
sspm0		equ	0

;----- ccp1con bits -------------------------------------------------------

ccp1x		equ	5
ccp1y		equ	4
ccp1m3		equ	3
ccp1m2		equ	2
ccp1m1		equ	1
ccp1m0		equ	0

;----- option bits --------------------------------------------------------

not_rbpu	equ	7	; rbpu is active low, but it's confusing to
rbpu		equ	7	; specify this so verbosely.
intedg		equ	6
t0cs		equ	5
t0se		equ	4
psa		equ	3
ps2		equ	2
ps1		equ	1
ps0		equ	0

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
sspie                        equ     h'0003'
ccp1ie                       equ     h'0002'
tmr2ie                       equ     h'0001'
tmr1ie                       equ     h'0000'

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

;	ram definition

	__maxram 0xbf
	__badram 0x0d, 0x18-0x1f, 0x8d, 0x8f-0x91,0x95-0x9f

;	configuration bits

_cp_all		equ	0x3f8f
_cp_75		equ	0x3f9f
_cp_50		equ	0x3faf
_cp_off		equ	0x3fbf
_pwrte_on	equ	0x3fbf
_pwrte_off	equ	0x3fb7
_wdt_on		equ	0x3fbf
_wdt_off	equ	0x3fbb
_lp_osc		equ	0x3fbc
_xt_osc		equ	0x3fbd
_hs_osc		equ	0x3fbe
_rc_osc		equ	0x3fbf
