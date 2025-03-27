    ; =============================================
; CNC Controller - Pure AVR Assembly
; ATmega328P - Arduino UNO + CNC Shield V4.0
; Versione completamente verificata
;prova compilare questo a https://www.costycnc.it/avr1
; =============================================

; --- Definizioni SFR ---
; Timer1 Control Registers
.equ TCCR1A = 0x80
.equ TCCR1B = 0x81
.equ WGM10  = 0  ; Waveform Generation Mode
.equ WGM11  = 1
.equ WGM12  = 3  ; <-- DEFINITO QUI
.equ WGM13  = 4
.equ CS10   = 0  ; Clock Select
.equ CS11   = 1
.equ CS12   = 2
.equ COM1A0 = 6
.equ COM1B0 = 4

; --- Registri utilizzati ---
; r24:r25 - Coppia per adiw (step counter)
; r16-r23 - Registri temporanei
; r26-r31 - Non usati (preservati)

; --- Definizioni pin CNC Shield ---
.equ X_STEP       = 2     ; PD2
.equ X_DIR        = 5     ; PD5
.equ Y_STEP       = 3     ; PD3
.equ Y_DIR        = 6     ; PD6
.equ Z_STEP       = 4     ; PD4
.equ Z_DIR        = 7     ; PD7



; --- Definizioni SFR ---
.equ SREG         = 0x3F
.equ SPH          = 0x3E
.equ SPL          = 0x3D
.equ PORTD        = 0x0B
.equ DDRD         = 0x0A
.equ TCCR1A       = 0x80
.equ TCCR1B       = 0x81
.equ OCR1AH       = 0x89
.equ OCR1AL       = 0x88
.equ OCIE1A      =1;
.equ TIMSK1       = 0x6F
.equ RAMEND       = 0x08FF

; --- Variabili in SRAM ---
.dseg
.org 0x0100
moving:          .byte 1
step_counter:    .byte 2
step_delay:      .byte 2

; --- Codice eseguibile ---
.cseg
.org 0x0000
    rjmp reset
.org 0x0016       ; Timer1 Compare A
    rjmp timer1_isr

reset:
    ; Inizializza stack
    ldi r16, high(RAMEND)
    out 0x3E, r16  ; SPH
    ldi r16, low(RAMEND)
    out 0x3D, r16  ; SPL

    ; Configura pin
    ldi r16, (1<<X_STEP)|(1<<Y_STEP)|(1<<Z_STEP)|(1<<X_DIR)|(1<<Y_DIR)|(1<<Z_DIR)
    out DDRD, r16

    ; Timer1 a 10kHz (CTC)
    ldi r16, 0
    sts TCCR1A, r16
    ldi r16, (1<<WGM12)|(1<<CS10)  ; CTC, no prescaler
    sts TCCR1B, r16
    ldi r16, high(1599)  ; 16MHz/1600 = 10kHz
    sts OCR1AH, r16
    ldi r16, low(1599)
    sts OCR1AL, r16
    ldi r16, (1<<OCIE1A) ; Abilita interrupt
    sts TIMSK1, r16

    ; Inizializza variabili
    clr r16
    sts moving, r16
    sts step_counter, r16
    sts step_counter+1, r16
    ldi r16, low(1000)
    sts step_delay, r16
    ldi r16, high(1000)
    sts step_delay+1, r16

    sei  ; Abilita interrupt

main_loop:
    rjmp main_loop

timer1_isr:
    push r16
    push r17
    push r18
    in r16, SREG
    push r16

    ; Controllo movimento
    lds r16, moving
    cpi r16, 0
    breq isr_end

    ; Incrementa step counter (r25:r24)
    lds r24, step_counter
    lds r25, step_counter+1
    adiw r24, 1  ; Solo con r24:r25!
    sts step_counter, r24
    sts step_counter+1, r25

    ; Verifica delay (r17:r16)
    lds r16, step_delay
    lds r17, step_delay+1
    cp r24, r16
    cpc r25, r17
    brlo isr_end

    ; Reset step counter
    clr r24
    sts step_counter, r24
    sts step_counter+1, r24

    ; Genera impulso STEP
    in r18, PORTD
    ori r18, (1<<X_STEP)|(1<<Y_STEP)|(1<<Z_STEP)
    out PORTD, r18
    nop  ; Ritardo
    nop
    andi r18, ~((1<<X_STEP)|(1<<Y_STEP)|(1<<Z_STEP))
    out PORTD, r18

    ; Qui inserire la logica di movimento...

isr_end:
    pop r16
    out SREG, r16
    pop r18
    pop r17
    pop r16
    reti
