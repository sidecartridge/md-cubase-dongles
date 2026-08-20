; User firmware module
; (C) 2026 by Diego Parrilla
; License: GPL v3
;
; Cubase dongle -- shipping behaviour. Reached via CMD_START (the [F]irmware
; menu command); by then the RP has committed to dongle mode: commemul is gone
; and the ROM3 state-machine engine (cubaseemul + Core1) is live, serving
; $FB0000 from the reset state.
;
; There is nothing for the m68k to do -- the dongle is answered entirely by the
; RP (PIO + Core1). So this just prints a one-line confirmation and returns,
; which hands control back to main.s's boot path and starts GEM. The dongle
; engine keeps running on Core1, so Cubase (launched from GEM) queries $FB0000
; and gets the real 5C060 responses.
;
; CMD_START is a one-way mode commit; reset the MultiDevice for the setup menu.
; Runs in place from ROM4 (romemul), reached after GEMDOS init (CA_INIT bit 27
; in main.s), so the Cconout trap below is safe.

	section text

GEMDOS_Cconout		equ 2	; print one char (char.w on stack)

userfw:
	lea	banner(pc), a0
	bsr	puts
	rts				; hand back to main.s -> boot GEM (the ROM3
					; dongle engine keeps running on Core1).

; puts: print the null-terminated string at A0 one char at a time (Cconout).
puts:
	move.b	(a0)+, d0
	beq.s	.done
	bsr	putc
	bra.s	puts
.done:
	rts

; putc: print D0.B via Cconout.
putc:
	move.w	d0, -(sp)
	move.w	#GEMDOS_Cconout, -(sp)
	trap	#1
	addq.l	#4, sp
	rts

banner:
	dc.b	27,"E"			; VT52 clear screen + home
	dc.b	"Cubase dongle active. Booting GEM -- launch Cubase.",13,10,0
	; firmware.py needs an even trimmed image length: a non-zero end marker at
	; an odd byte keeps it even regardless of the string above.
	even
	dc.w	$FFFF
