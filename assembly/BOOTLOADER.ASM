; CP/M bootloader stub for 64K map
; Assembler: z80asm
;
; Flow:
; 1) Initialize stack pointer to FFFFh
; 2) Configure RAM in banks 1,2 and 3
; 3) Copy BIOS binary block to F200h
; 4) Copy 2nd stage Bootloader to 04000H
; 5) Jump to 2nd stage

			ORG 0000H

STACK_TOP:         EQU 0FFFFH
PORT_BANK_0_CTRL:  EQU 090H
PORT_BANK_1_CTRL:  EQU 091H
PORT_BANK_2_CTRL:  EQU 092H
PORT_BANK_3_CTRL:  EQU 093H
RAM_0_BANK_VALUE:  EQU 00H      ; not used in 1st stage boorloader
RAM_1_BANK_VALUE:  EQU 01H
RAM_2_BANK_VALUE:  EQU 02H
SYSTEM_BANK_VALUE: EQU 0FEH     ; decimal 254

BIOS_DEST:        EQU 0F200H
BOOT2_DEST:		  EQU 04000H

; -----------------------------------------------------------------------------
; RESET_ENTRY: CPU reset vector at 0000h
; -----------------------------------------------------------------------------

RESET_ENTRY:
			LD SP,STACK_TOP

			LD A,RAM_1_BANK_VALUE
			OUT (PORT_BANK_1_CTRL),A

			LD A,RAM_2_BANK_VALUE
			OUT (PORT_BANK_2_CTRL),A

			LD A,SYSTEM_BANK_VALUE
			OUT (PORT_BANK_3_CTRL),A

			; Load CP/M BIOS 

			LD HL,BIOS_IMAGE
			LD DE,BIOS_DEST
			LD BC,BIOS_IMAGE_END-BIOS_IMAGE
			LDIR

			; Load 2nd stage bootloader
			LD HL,BOOT2_IMAGE
			LD DE,BOOT2_DEST
			LD BC,BOOT2_IMAGE_END-BOOT2_IMAGE
			LDIR

; -----------------------------------------------------------------------------
; BIOS_IMAGE: binary BIOS payload copied into RAM at F200h
; -----------------------------------------------------------------------------

BIOS_IMAGE:
			INCBIN "BIOS.BIN"
BIOS_IMAGE_END:

BOOT2_IMAGE:
			INCBIN "BOOT2STAGE.BIN"
BOOT2_IMAGE_END: