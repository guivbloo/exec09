; helloworld.S
; This program sends "Hello World !" on the serial output using ACIA m6850
; No usage of interrupt or stack

ACIA_CFG_REG	equ $C000
ACIA_STATUS_REG	equ $C000
ACIA_DATA_REG   equ $C001

ACIA_RESET	equ %00000011	; CR0 = 1 and CR1 = 1
ACIA_CFG	equ %00010101	; Receive Int disable, Transmit Int disable, 8bits + 1 stop, clock/4/16=115200
ACIA_TDRE_MASK	equ %00000010	; Mask to test Transmit Data Register Empty
ACIA_RDRF_MASK	equ %00000001	; Mask to test Receive Data Register Full
		
	org 0xFF00
main:
	orcc #$FF       	; Disable interrupts.
	clra		
	tfr a,dp        	; Set direct page register to 0.
        ldb #ACIA_RESET
	stb ACIA_CFG_REG	; Master reset
	ldb #ACIA_CFG
	stb ACIA_CFG_REG	; ACIA config via control register
	ldx #helloworld
putsagain:
	ldb ,x+
	beq putsend
tdr_full:
	lda ACIA_STATUS_REG
	anda #ACIA_TDRE_MASK
	beq tdr_full
	stb ACIA_DATA_REG
	bra putsagain	
putsend:
	bra .
helloworld:        
        fcc 'Hello World !'
	fcb $0D,$0A,$00
		
        org 0xFFFE    
reset:	fdb main		; RESET
