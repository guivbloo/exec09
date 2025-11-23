

;**************** Constants ****************

stackstart 	equ $B000
csw_start 	equ $1000
rom_start	equ $E000
ACIA_RESET	equ %00000011		; CR0 = 1 and CR1 = 1
ACIA_CFG	equ %10010101		; Receive Int enable, Transmit Int disable, 8bits + 1 stop, clock/4/16=115200
ACIA_TDRE_MASK	equ %00000010		; Mask to test Transmit Data Register Full
ACIA_RDRF_MASK	equ %00000001		; Mask to test Receive Data Register Full
ACIA_CFG_REG	equ $C000
ACIA_STATUS_REG	equ $C000
ACIA_DATA_REG   equ $C001

K_INTER 	equ '?
K_HASH		equ '#
K_EXC		equ '!
K_AND		equ '&


		org 0x0000
SWI3VEC:	rmb 2			;SWI3 vector
SWI2VEC:	rmb 2			;SWI2 vector
FIRQVEC:	rmb 2			;FIRQ vector
IRQVEC:		rmb 2			;IRQ vector
SWIVEC:		rmb 2			;SWI vector
NMIVEC:		rmb 2			;NMI vector
temp		rmb 2			;var temp
temp2		rmb 1


		org 0xE000
csw_entry:	
		bra .

		org 0xFE00
main:		orcc #$FF       	; Disable interrupts.
		clra		
		tfr a,dp        	; Set direct page register to 0.
		lds #stackstart   	; Initialize HwStack
		jsr uart_init
		ldx #welcome
		jsr uart_puts
		ldb #$11	 	; Sends 0x11
go6:		jsr uart_putc	
		beq go6
		jsr uart_kbhit
		beq endflash
		jsr uart_getc
		cmpb #$11		; did we receive 0x11 ?
		bne endflash
go3:		jsr uart_kbhit
		beq go3
		jsr uart_getc
		exg A,B
go4:		jsr uart_kbhit
		beq go4
		jsr uart_getc
		addd #csw_start
		std temp
go5:		jsr uart_kbhit
		beq go5
		jsr uart_getc	
		stb temp2	
		clra
nextblock:	ldx #csw_start		; init x to point on RAM
go1:		jsr uart_kbhit		; transfert CSW
		beq go1
		jsr uart_getc
		stb ,x+
		pshs b
		adda ,s+
		cmpx temp
		bne go1

		cmpa temp2		; check integrity
		bne endflash

		ldu #rom_start		; init u to point on ROM
		ldx #csw_start		; 
go2:		lda ,x+			; get the byte from ram
		sta ,u+			; and write it into the rom
		cmpx temp		; 
		bne go2			; 

		ldb #K_AND 		; Sends '&'
		jsr uart_putc
		ldx #ok			;Flash done
		jsr uart_puts
		jmp csw_entry
endflash:	ldx #ko			;Flash ko
		jsr uart_puts
		bra .

;***************** Subroutines to manage UART ***************
;Init ACIA
uart_init:	pshs b
		ldb #ACIA_RESET
		stb ACIA_CFG_REG	;Master reset
		ldb #ACIA_CFG
		stb ACIA_CFG_REG	;ACIA config via control register
		puls b
		rts
		
;Write a caracter to ACIA
;Caracter to be written is in B
uart_putc:  	pshs a
tdr_full:	lda ACIA_STATUS_REG
		anda #ACIA_TDRE_MASK
		beq tdr_full
		stb ACIA_DATA_REG
		puls a
		rts	
		
;Return 1 in B if a character is available 0 otherwise
uart_kbhit:	pshs a
		ldb #$00
		lda ACIA_STATUS_REG
		anda #ACIA_RDRF_MASK
		beq rdrf_empty
		incb
rdrf_empty:	puls a
		rts

;String to be sent is pointed by X
uart_puts:	pshs b
putsagain:	ldb ,x+
		beq putsend
		bsr uart_putc
		bra putsagain
putsend:	puls b
		rts
				
;Read a caracter in ACIA
;Caracter is put in B
uart_getc:
		ldb ACIA_DATA_REG
		rts

;************ Strings ************
welcome:        fcc '6809 RSW v0.1-alpha1'
		fcb $0D,$0A,$00
ok:		fcc 'Update OK'
		fcb $0D,$0A,$00

ko:		fcc 'Update KO'
		fcb $0D,$0A,$00

;************ Interrupt handlers ************
SWI3_HANDLER:	jmp	[SWI3VEC]
SWI2_HANDLER:	jmp	[SWI2VEC]
FIRQ_HANDLER:	jmp	[FIRQVEC]
IRQ_HANDLER:	jmp	[IRQVEC]
SWI_HANDLER:	jmp	[SWIVEC]
NMI_HANDLER:	jmp	[NMIVEC]
			
;************ Interrupt vectors **********
		org 0xFFF0
		fdb	0x0000			;reserved
		org 0xFFF2
		fdb	SWI3_HANDLER
		org 0xFFF4
		fdb	SWI2_HANDLER
		org 0xFFF6
		fdb	FIRQ_HANDLER
		org 0xFFF8
		fdb	IRQ_HANDLER
		org 0xFFFA
		fdb	SWI_HANDLER
		org 0xFFFC
		fdb	NMI_HANDLER
		org 0xFFFE
RESET:		fdb	main
