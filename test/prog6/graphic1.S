; This programs display a pattern on the screen in mode 1 (graphics 1)

ACIA_CFG_REG	equ $C000
ACIA_STATUS_REG	equ $C000
ACIA_DATA_REG   equ $C001

TMS_REG0_DATA	equ $00	; Ext OUT disabled
TMS_REG1_DATA	equ $C0	; 16K, Enable Display, Disable Int.,G1 mode, 8x8 Sprites, Mag. Off
TMS_REG2_DATA	equ $05 ; Start address of name table blocks R2*0x400 -> 0x1400
TMS_REG3_DATA	equ $80 ; Start address of color table blocks R3*0x40 -> 0x2000
TMS_REG4_DATA	equ $01 ; Start address of pattern table blocks R4*0x800 -> 0x0800
TMS_REG5_DATA	equ $20 ; Start address of sprite attribute table blocks R5*0x80 -> 0x1000
TMS_REG6_DATA	equ $00	; Start address of sprite pattern table blocks R6*0x800 -> 0x0000
TMS_REG7_DATA 	equ $01	; Background color = Black

TMS_BASE_0		equ $C800	; Mode = 0
TMS_BASE_1		equ $C801	; Mode = 1
TMS_REG0_ADDR	equ $80	; TMS9918 Register 0 address
TMS_REG1_ADDR	equ $81	; TMS9918 Register 1 address
TMS_REG2_ADDR	equ $82	; TMS9918 Register 2 address
TMS_REG3_ADDR	equ $83	; TMS9918 Register 3
TMS_REG4_ADDR	equ $84	; TMS9918 Register 4
TMS_REG5_ADDR	equ $85	; TMS9918 Register 5
TMS_REG6_ADDR	equ $86	; TMS9918 Register 6
TMS_REG7_ADDR	equ $87	; TMS9918 Register 7

ACIA_RESET	equ %00000011	; CR0 = 1 and CR1 = 1
ACIA_CFG	equ %10010101	; Receive Int enable, Transmit Int disable, 8bits + 1 stop, clock/4/16=115200
ACIA_TDRE_MASK	equ %00000010	; Mask to test Transmit Data Register Empty
ACIA_RDRF_MASK	equ %00000001	; Mask to test Receive Data Register Full

RAMSTART	equ $1000


    org 0xE000

    include "pat.inc"

main:
	andcc #%11101111       	; Enable IRQ interrupt.
	clra		
	tfr a,dp        	; Set direct page register to 0.
        lds #RAMSTART   	; Initialize HwStack
        ldb #ACIA_RESET
	stb ACIA_CFG_REG	;Master reset
	ldb #ACIA_CFG
	stb ACIA_CFG_REG	;ACIA config via control register

	ldb #$23		; "#"
	jsr acia_putc
	ldb #TMS_REG0_DATA
	stb TMS_BASE_1
	ldb #TMS_REG0_ADDR
	stb TMS_BASE_1		; Write to TMS register 0
	ldb #TMS_REG1_DATA
	stb TMS_BASE_1
	ldb #TMS_REG1_ADDR
	stb TMS_BASE_1		; Write to TMS register 1
	ldb #TMS_REG2_DATA
	stb TMS_BASE_1
	ldb #TMS_REG2_ADDR
	stb TMS_BASE_1		; Write to TMS register 2
	ldb #TMS_REG3_DATA
	stb TMS_BASE_1
	ldb #TMS_REG3_ADDR
	stb TMS_BASE_1		; Write to TMS register 3
	ldb #TMS_REG4_DATA
	stb TMS_BASE_1
	ldb #TMS_REG4_ADDR
	stb TMS_BASE_1		; Write to TMS register 4
	ldb #TMS_REG5_DATA
	stb TMS_BASE_1
	ldb #TMS_REG5_ADDR
	stb TMS_BASE_1		; Write to TMS register 5
	ldb #TMS_REG6_DATA
	stb TMS_BASE_1
	ldb #TMS_REG6_ADDR
	stb TMS_BASE_1		; Write to TMS register 6
	ldb #TMS_REG7_DATA
	stb TMS_BASE_1
	ldb #TMS_REG7_ADDR
	stb TMS_BASE_1		; Write to TMS register 7

InitPattern:
    LDX  #B_PatternTable
	LDB #$00
	STB TMS_BASE_1		 ; Adresse haute
	LDA #$48			 ; Adresse basse
	STA TMS_BASE_1         ; Adresse basse

CopyPatternLoop:
    LDA ,X+					; lire un byte du charset
	STA TMS_BASE_0         ; écrire dans la VRAM
	CMPX  #E_PatternTable   ; Comparer X à EndCharsetData
    BNE CopyPatternLoop		; non, continuer la copie

	LDX #B_NameTable      ; Pointeur vers la chaîne "Hello World!"
	LDB #$00
	STB TMS_BASE_1		 ; Adresse haute
	LDA #$54			 ; Adresse basse
	STA TMS_BASE_1         ; Adresse basse
DisplayLoop:
	LDA ,X+             ; Charger le caractère suivant
	STA TMS_BASE_0     ; Écrire le caractère dans la VRAM
	CMPX #E_NameTable         ; Vérifier si c'est la fin de la chaîne
	BNE DisplayLoop


    LDX  #B_ColorTable
	LDB #$00
	STB TMS_BASE_1		 ; Adresse haute
	LDA #$60			 ; Adresse basse
	STA TMS_BASE_1         ; Adresse basse

CopyColorLoop:
    LDA ,X+					; lire un byte du charset
	STA TMS_BASE_0         ; écrire dans la VRAM
	CMPX  #E_ColorTable   ; Comparer X à EndCharsetData
    BNE CopyColorLoop		; non, continuer la copie

	BRA .
	
;Write a caracter to ACIA
;Caracter to be written is in B
acia_putc:  
	pshs a
tdr_full:
	lda ACIA_STATUS_REG
	anda #ACIA_TDRE_MASK
	beq tdr_full
	stb ACIA_DATA_REG
	puls a
	rts	

irq_handler:
	lda ACIA_STATUS_REG	; Check IRQ bit of ACIA status
	bpl end_inter		; branch if O
	lsra			; Check RDRF bit of ACIA
	bcc end_inter		; branch if 0
	ldb ACIA_DATA_REG
	cmpb #$3f		; Did we receive '?'
	bne end_inter
	ldb #$21		; yes, send "!"
	bsr acia_putc		;
end_inter:
	rti

			
	org 0xFFF8
	fdb irq_handler		; 
	org 0xFFFE 
reset:	fdb main		; RESET