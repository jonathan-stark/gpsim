
;;
;;	Include file for the p16c71
;;

;----- register files------------------------------------------------------

indf		equ	0x0
tmr0		equ	0x1
pcl		equ	0x2
status		equ	0x3
fsr		equ	0x4
porta		equ	0x5
portb		equ	0x6

adcon0		equ	0x8
adres		equ	0x9
pclath		equ	0xa
intcon		equ	0xb

option_reg	equ	0x81
trisa		equ	0x85
trisb		equ	0x86
adcon1		equ	0x88


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
adie		equ	6
t0ie		equ	5
inte		equ	4
rbie		equ	3
t0if		equ	2
intf		equ	1
rbif		equ	0

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

;----- adcon0 bits --------------------------------------------------------

adcs1		equ	7
adcs0		equ	6
chs1		equ	4
chs0		equ	3
go		equ	2
adif		equ	1
adon		equ	0

;----- adcon1 bits --------------------------------------------------------

pcfg1		equ	1
pcfg0		equ	0


;	ram definition

	__maxram 0xaf
	__badram 0x07, 0x30-0x7f, 0x87

;	configuration bits

_cp_on		equ	0x3fef
_cp_off		equ	0x3fff
_pwrte_on	equ	0x3fff
_pwrte_off	equ	0x3ff7
_wdt_on		equ	0x3fff
_wdt_off	equ	0x3ffb
_lp_osc		equ	0x3ffc
_xt_osc		equ	0x3ffd
_hs_osc		equ	0x3ffe
_rc_osc		equ     0x3fff

