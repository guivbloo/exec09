; Voici un exemple clair et utile de ce que tu peux mettre dans un fichier
; memorymap.inc pour un projet 6809 assemblé avec LWTOOLS.
; Ce fichier doit contenir uniquement des définitions d’adresses, constantes, tailles et zones mémoire
; aucun code, aucune directive section.

; Pourquoi ce fichier ?
; Parce que tu veux :
; - centraliser toutes les adresses matérielles
; - éviter les duplications entre drivers
; - pouvoir changer facilement l'adresse d’un périphérique sans modifier 20 fichiers
; - rendre ton projet portable à une autre carte

; Comment l’utiliser ?
; Par exemple dans ton driver ACIA :
;        include "memorymap.inc"
;
;ACIA_KbHit:
;        lda     ACIA6850_STATUS
;        anda    #$01
;        rts
;
; Ou dans ton programme principal :
;        include "memorymap.inc"
;
;Start:
;        lda     #$55
;        sta     RAM_START       ; stocke dans le début de la RAM


; ============================
;   MEMORY MAP – 6809 Project
; ============================

; --- RAM ---
RAM_START       equ   $0000
STACK_TOP       equ   $00FF     ; pile si tu veux utiliser la page zéro
SYSTEM_VARS     equ   $0100     ; zone pour variables système
RAM_END         equ   $7FFF

; --- E/S MAPPÉES (MMIO) ---
; Modifier selon ta carte !
ACIA6850_BASE   equ   $A000
ACIA6850_STATUS equ   ACIA6850_BASE       ; statut
ACIA6850_DATA   equ   ACIA6850_BASE + 1   ; data register
ACIA6850_CTRL   equ   ACIA6850_BASE + 2   ; selon ton câblage (optionnel)

; --- ROM ---
ROM_START       equ   $8000
ROM_END         equ   $FFFF

; --- VECTEURS D'INTERRUPTION (Fixés par le 6809) ---
VECTOR_RESET    equ   $FFFE
VECTOR_NMI      equ   $FFFC
VECTOR_SWI      equ   $FFFA
VECTOR_IRQ      equ   $FFF8
VECTOR_FIRQ     equ   $FFF6
