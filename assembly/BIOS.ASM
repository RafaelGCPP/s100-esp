; CP/M 2.2 BIOS skeleton for S-100 ESP prototype
; Assembler: z80asm
; Scope: boot + console implemented, printer/punch/reader stubbed, disk entry points stubbed
;
; Notes:
; - This file is intentionally independent from CMake.
; - Build it manually with z80asm in your preferred script/workflow.
; - Addresses for CCP/BDOS vectors are placeholders and should be adjusted
;   once your memory map and loader flow are finalized.
;
; -----------------------------------------------------------------------------
; Constants
; -----------------------------------------------------------------------------

UART_BASE:       EQU 00
UART_DATA:       EQU UART_BASE + 0
UART_LSR:        EQU UART_BASE + 5

LSR_DR:          EQU 01H     ; Data Ready
LSR_THRE:        EQU 20H     ; Transmit Holding Register Empty

EOF_CHAR:        EQU 1AH

; CP/M page-0 vectors (project-specific placeholders)
WBOOT_VECTOR:    EQU 0000H
BDOS_VECTOR:     EQU 0005H

; Replace these with your actual runtime layout
CCP_ENTRY:       EQU 3400H
BDOS_ENTRY:      EQU 3A00H

; -----------------------------------------------------------------------------
; BIOS jump table (17 x JP = 51 bytes)
; -----------------------------------------------------------------------------

            ORG 0F200H

BOOT:       JP COLD_START
WBOOT:      JP WARM_START
CONST:      JP CONSOLE_STATUS
CONIN:      JP CONSOLE_INPUT
CONOUT:     JP CONSOLE_OUTPUT
LIST:       JP LIST_OUTPUT
PUNCH:      JP PUNCH_OUTPUT
READER:     JP READER_INPUT
HOME:       JP DISK_HOME
SELDSK:     JP DISK_SELECT
SETTRK:     JP DISK_SET_TRACK
SETSEC:     JP DISK_SET_SECTOR
SETDMA:     JP DISK_SET_DMA
READ:       JP DISK_READ
WRITE:      JP DISK_WRITE
LISTST:     JP LIST_STATUS
SECTRAN:    JP SECTOR_TRANSLATE

; -----------------------------------------------------------------------------
; BIOS state variables
; -----------------------------------------------------------------------------

CURDRV:         DB 00H
CURTRK:         DW 0000H
CURSEC:         DW 0000H
CURDMA:         DW 0080H

; -----------------------------------------------------------------------------
; 0: BOOT - cold start entry
; Initializes vectors and jumps to CCP.
; -----------------------------------------------------------------------------

COLD_START:
            ; Page-0 warm boot vector: JP WBOOT
            LD A,0C3H
            LD (WBOOT_VECTOR),A
            LD HL,WARM_START
            LD (WBOOT_VECTOR+1),HL

            ; Page-0 BDOS entry vector: JP BDOS_ENTRY
            LD A,0C3H
            LD (BDOS_VECTOR),A
            LD HL,BDOS_ENTRY
            LD (BDOS_VECTOR+1),HL

            ; TODO: initialize UART, disk controller, and memory mapper if needed

            LD C,00H          ; default drive A:
            JP CCP_ENTRY

; -----------------------------------------------------------------------------
; 1: WBOOT - warm start entry
; Restores vectors and jumps to CCP.
; -----------------------------------------------------------------------------

WARM_START:
            ; Reinstall page-0 vectors
            LD A,0C3H
            LD (WBOOT_VECTOR),A
            LD HL,WARM_START
            LD (WBOOT_VECTOR+1),HL

            LD A,0C3H
            LD (BDOS_VECTOR),A
            LD HL,BDOS_ENTRY
            LD (BDOS_VECTOR+1),HL

            ; TODO: reload CCP/BDOS from system tracks when disk path exists

            LD BC,(CURDRV)
            JP CCP_ENTRY

; -----------------------------------------------------------------------------
; 2: CONST - console status
; Returns A=FFH if char available, else A=00H.
; -----------------------------------------------------------------------------

CONSOLE_STATUS:
            IN A,(UART_LSR)
            AND LSR_DR
            JR Z,CONST_NO_CHAR
            LD A,0FFH
            RET

CONST_NO_CHAR:
            XOR A
            RET

; -----------------------------------------------------------------------------
; 3: CONIN - blocking console input
; Waits for a character and returns it in A (bit 7 cleared).
; -----------------------------------------------------------------------------

CONSOLE_INPUT:
CONIN_WAIT:
            IN A,(UART_LSR)
            AND LSR_DR
            JR Z,CONIN_WAIT
            IN A,(UART_DATA)
            AND 7FH
            RET

; -----------------------------------------------------------------------------
; 4: CONOUT - console output
; Character to transmit is in C.
; -----------------------------------------------------------------------------

CONSOLE_OUTPUT:
CONOUT_WAIT:
            IN A,(UART_LSR)
            AND LSR_THRE
            JR Z,CONOUT_WAIT
            LD A,C
            OUT (UART_DATA),A
            RET

; -----------------------------------------------------------------------------
; 5: LIST - list/printer output
; Stub: drop character and return.
; -----------------------------------------------------------------------------

LIST_OUTPUT:
            RET

; -----------------------------------------------------------------------------
; 6: PUNCH - punch output
; Stub: drop character and return.
; -----------------------------------------------------------------------------

PUNCH_OUTPUT:
            RET

; -----------------------------------------------------------------------------
; 7: READER - reader input
; Stub: return EOF character.
; -----------------------------------------------------------------------------

READER_INPUT:
            LD A,EOF_CHAR
            RET

; -----------------------------------------------------------------------------
; 8: HOME - move head to track 0
; Stub for now.
; -----------------------------------------------------------------------------

DISK_HOME:
            RET

; -----------------------------------------------------------------------------
; 9: SELDSK - select disk
; Input: C=drive number, E=select mode
; Output: HL=DPH pointer or 0000H if invalid
; Stub returns 0000H for now.
; -----------------------------------------------------------------------------

DISK_SELECT:
            LD A,C
            LD (CURDRV),A
            LD HL,0000H
            RET

; -----------------------------------------------------------------------------
; 10: SETTRK - set track for next I/O
; Input: BC=track
; -----------------------------------------------------------------------------

DISK_SET_TRACK:
            LD (CURTRK),BC
            RET

; -----------------------------------------------------------------------------
; 11: SETSEC - set sector for next I/O
; Input: BC=sector
; -----------------------------------------------------------------------------

DISK_SET_SECTOR:
            LD (CURSEC),BC
            RET

; -----------------------------------------------------------------------------
; 12: SETDMA - set DMA address for next I/O
; Input: BC=DMA address
; -----------------------------------------------------------------------------

DISK_SET_DMA:
            LD (CURDMA),BC
            RET

; -----------------------------------------------------------------------------
; 13: READ - read one logical sector
; Output: A=00 success, A=01 error
; Stub returns error for now.
; -----------------------------------------------------------------------------

DISK_READ:
            LD A,01H
            RET

; -----------------------------------------------------------------------------
; 14: WRITE - write one logical sector
; Input: C=write type
; Output: A=00 success, A=01 error, A=02 write-protect
; Stub returns error for now.
; -----------------------------------------------------------------------------

DISK_WRITE:
            LD A,01H
            RET

; -----------------------------------------------------------------------------
; 15: LISTST - list device status
; Returns A=FFH (always ready).
; -----------------------------------------------------------------------------

LIST_STATUS:
            LD A,0FFH
            RET

; -----------------------------------------------------------------------------
; 16: SECTRAN - logical to physical sector translation
; Input: BC=logical sector, DE=table pointer (or 0000H)
; Output: HL=physical sector
; If DE=0000H, returns BC unchanged.
; -----------------------------------------------------------------------------

SECTOR_TRANSLATE:
            LD A,D
            OR E
            JR Z,SECTRAN_IDENTITY

            EX DE,HL
            ADD HL,BC
            LD A,(HL)
            LD H,00H
            LD L,A
            RET

SECTRAN_IDENTITY:
            LD H,B
            LD L,C
            RET
