; User firmware module
; (C) 2026 by Diego Parrilla
; License: GPL v3
;
; EPIC-04 gate #1 (ROM3-drive test). main.s hands control here when the RP
; signals CMD_START via the cartridge sentinel (the [F]irmware menu command);
; the cartridge image places this file at offset $0800 (USERFW = $FA0800).
;
; It reads four words from the ROM3 window ($FB0000..) and prints them as hex.
; The RP drives a fixed word on ROM3 (cubaseemul), so every word must read back
; as FEFF. If they read FFFF / random / varying, the MultiDevice is not driving
; ROM3 data (docs/epics/DECISIONS.md D-07).
;
; This runs in place from the cartridge ROM (ROM4). Executing from ROM4 while
; reading ROM3 is fine — the command protocol (send_sync) does exactly that all
; the time. It is reached via CMD_START (the [F]irmware menu command), which is
; a one-way mode commit: after the keypress it returns into the boot path (GEM),
; not the setup menu. Reset the MultiDevice to get the menu back.
;
; The cartridge is reached after GEMDOS init (CA_INIT bit 27 in main.s's
; header), so the traps below are safe.

	section text

GEMDOS_Cconout		equ 2	; print one char (char.w on stack)
GEMDOS_Cnecin		equ 8	; wait for a key, no echo

userfw:
	lea	header(pc), a0
	bsr	puts

	move.w	$FB0000, d0			; ROM3 reads (in place; ROM4 exec is fine)
	bsr	put_word_hex
	move.w	$FB0002, d0
	bsr	put_word_hex
	move.w	$FB0004, d0
	bsr	put_word_hex
	move.w	$FB0006, d0
	bsr	put_word_hex

	lea	footer(pc), a0
	bsr	puts

	move.w	#GEMDOS_Cnecin, -(sp)		; keep the result on screen until a key
	trap	#1
	addq.l	#2, sp
	rts					; hands off (boots GEM) — CMD_START is a
						; one-way mode commit; reset the MultiDevice
						; to get the setup menu back.

; put_word_hex: print D0.W as 4 uppercase hex digits followed by a space.
put_word_hex:
	movem.l	d5-d7, -(sp)
	move.w	d0, d5
	moveq	#12, d6				; nibble shift: 12, 8, 4, 0
.nibble:
	move.w	d5, d7
	lsr.w	d6, d7
	and.w	#$0F, d7
	add.w	#'0', d7
	cmp.w	#'9', d7
	ble.s	.emit
	addq.w	#7, d7				; gap between '9' and 'A'
.emit:
	move.w	d7, d0
	bsr	putc
	subq.w	#4, d6
	bpl.s	.nibble
	moveq	#' ', d0
	bsr	putc
	movem.l	(sp)+, d5-d7
	rts

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

header:
	dc.b	27,"E"				; VT52 clear screen + home
	dc.b	"ROM3 peek $FB0000 (expect FEFF x4):",13,10,0
footer:
	dc.b	13,10,"ROM3 drive confirmed. A key boots GEM;",13,10
	dc.b	"reset the MultiDevice for the setup menu.",13,10,0
	; firmware.py trims trailing zeros and needs a whole number of 16-bit
	; words. Align even, then a non-zero marker at the odd byte so the trimmed
	; image length is always even regardless of the strings above.
	even
	dc.w	$FFFF
