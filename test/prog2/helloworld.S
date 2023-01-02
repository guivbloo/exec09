; helloworld.S
; This program sends "Hello World !" on the serial output using ACIA m6850
; then sends '#'' and expect '?'. If '?' is received it sends '!'
; Usage of interrupt and stack

ACIA_CFG_REG	equ $C000
ACIA_STATUS_REG	equ $C000
ACIA_DATA_REG   equ $C001

ACIA_RESET	equ %00000011	; CR0 = 1 and CR1 = 1
ACIA_CFG	equ %10010101	; Receive Int enable, Transmit Int disable, 8bits + 1 stop, clock/4/16=115200
ACIA_TDRE_MASK	equ %00000010	; Mask to test Transmit Data Register Empty
ACIA_RDRF_MASK	equ %00000001	; Mask to test Receive Data Register Full

RAMSTART	equ $1000
		
	org 0xFF00
main:
	andcc #%11101111       	; Enable IRQ interrupt.
	clra		
	tfr a,dp        	; Set direct page register to 0.
        lds #RAMSTART   	; Initialize HwStack
        ldb #ACIA_RESET
	stb ACIA_CFG_REG	;Master reset
	ldb #ACIA_CFG
	stb ACIA_CFG_REG	;ACIA config via control register

	ldx #helloworld
putsagain:
	ldb ,x+
	beq putsend
	bsr acia_putc
	bra putsagain
putsend:
	ldb #$23		; "#"
	bsr acia_putc
	bra .			; Endless loop

		
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

		
helloworld:        
        fcc 'Hello World !'
	fcb $0D,$0A,$00
		
	org 0xFFF8
	fdb irq_handler		; 
	org 0xFFFE 
reset:	fdb main		; RESET
