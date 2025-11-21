; ptm.S
; This program configures a timer of 1 second and sends '@' 
; on the ACIA every second
; Usage of interrupt and stack


COMPT			equ $A000
PTM_CTRL_REG1	equ $C400
PTM_CTRL_REG2	equ $C401
PTM_CTRL_REG3	equ $C400
PTM_MSB_REG		equ $C404
PTM_LATCH2_REG	equ $C405
PTM_STS_REG		equ $C401
PTM_CFG_2		equ %01000011	;Continuous mode, 16bits mode, E clock source, no reset
ACIA_CFG_REG	equ $C000
ACIA_STATUS_REG	equ $C000
ACIA_DATA_REG   equ $C001

ACIA_RESET	equ %00000011	; CR0 = 1 and CR1 = 1
ACIA_CFG	equ %10010101	; Receive Int enable, Transmit Int disable, 8bits + 1 stop, clock/4/16=115200
ACIA_TDRE_MASK	equ %00000010	; Mask to test Transmit Data Register Full
ACIA_RDRF_MASK	equ %00000001	; Mask to test Receive Data Register Full
RAMSTART	equ $1000
		
	org 0xE000
main:
	andcc #%11101111       	; Enable IRQ interrupt.
	clra		
	tfr a,dp        ; Set direct page register to 0.
    lds #RAMSTART   ; Initialize HwStack

	ldb COMPT
	clrb 			; Clear compt
	incb 		;
	stb COMPT		; COMPT = 1
	ldx #$ffFE
	stx PTM_MSB_REG
	ldb #PTM_CFG_2
	stb PTM_CTRL_REG2 
	bra .

		
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
	lda PTM_STS_REG		; Check IRQ bit of PTM status
	bpl end_inter		; branch if O
	lsra
	lsra			; Check if it is Timer 2
	bcc end_inter		; branch if 0
	ldx PTM_MSB_REG
	ldb COMPT
	incb
	stb COMPT		; compt++
	cmpb #$64
	beq send_at		; if compt == 5, send "@"
	bra end_inter
send_at:
	clrb
	stb COMPT		; compt = 0
	ldb #$40		; yes, send "@"
	bsr acia_putc		;
	

end_inter:
	rti

	org 0xFFF8
	fdb irq_handler		; 	
        org 0xFFFE    
reset:	fdb main		;RESET
