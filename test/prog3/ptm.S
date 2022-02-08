; ptm.S
; This program configures a timer of 1 second and sends '#'' 
; on the ACIA every second
; Usage of interrupt and stack

PTM_CTRL_REG1	equ $C080
PTM_CTRL_REG2	equ $C081
PTM_CTRL_REG3	equ $C080
PTM_CFG		equ %01000010	;16bits mode, E clock source, no reset
RAMSTART	equ $1000
		
	org 0xFF00
main:
	orcc #$FF       ; Disable interrupts.
	clra		
	tfr a,dp        ; Set direct page register to 0.
        lds #RAMSTART   ; Initialize HwStack

	ldb #PTM_CFG
	stb PTM_CTRL_REG 
	ldx #helloworld
putsagain:
	ldb ,x+
	beq putsend
	bsr acia_putc
	bra putsagain
putsend:
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
		
		
        org 0xFFFE    
reset:	fdb main	;RESET
