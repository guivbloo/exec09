; Ce fichier sert à :
; faire l'initialisation minimale du système
; préparer la RAM, la pile, et éventuellement les périphériques essentiels
; transférer la main à ton main ou Start
; Il contient généralement :
; le Reset_Handler
; éventuellement des routines d’init pour les périphériques (IRQ, ACIA, PIA…)
; rien d’autre

; ==============================
;   init.asm – System startup
; ==============================

        INCLUDE "memorymap.inc"

        SECTION .text

        GLOBAL Reset_Handler

; ---------------------------------------------------------
;  RESET HANDLER
; ---------------------------------------------------------
Reset_Handler:

        ; désactive les IRQ pendant l'init
        orcc    #$50        ; I=1 F=1 (IRQ+FIRQ off)

        ; -------------------------------------------------
        ; Initialiser la pile
        ; -------------------------------------------------
        lds     #STACK_TOP

        ; -------------------------------------------------
        ; (Optionnel) effacer la RAM si on veut une RAM propre
        ; -------------------------------------------------
        ; ldx #RAM_START
        ; lda #0
.clear_ram:
        ; sta ,x+
        ; cmpx #RAM_END
        ; bne .clear_ram

        ; -------------------------------------------------
        ; INIT DES PÉRIPHÉRIQUES (exemple ACIA 6850)
        ; -------------------------------------------------
        jsr     Init_ACIA

        ; activer les IRQ si nécessaire
        andcc   #$AF        ; I=0 F=0 (réactive IRQ+FIRQ)

        ; passer au programme principal
        jmp     Start        ; Start est global dans main.asm


; ---------------------------------------------------------
;  INIT ACIA 6850 — Exemple simple
; ---------------------------------------------------------
        GLOBAL Init_ACIA

Init_ACIA:
        ; Ici tu mets la config réelle de ton 6850 selon ton câblage
        ; Par exemple :
        lda     #%00010101      ; 8N1, /16 clock (à adapter)
        sta     ACIA6850_CTRL
        rts
