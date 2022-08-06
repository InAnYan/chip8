// chip8-assembler v0.7 - assembler written in educational purposes
// Uses Cowgods Chip-8 Technical Reference v1.0
// http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
// Features: mark support, dw command with mark as constant variable
// Stability and correctness are not guaranteed

jp [main] // jump to the address of mark

// 0000 1110 - does not support multiline comments
// 0011 0000
// 0011 0000
// 0011 0000
// 0000 1110
C_SPRITE:
    dw 0x0e30 // dw command is always 2 bytes long
    dw 0x3030
    dw 0x0e00

; 0110 0110 - multiple comments style
; 0110 0110
; 0111 1110
; 0110 0110
; 0110 0110
H_SPRITE:
    dw 0x6666
    dw 0x7e66
    dw 0x6600

-- 0011 1100
-- 0001 1000
-- 0001 1000
-- 0001 1000
-- 0011 1100
I_SPRITE:
    dw 0x3c18
    dw 0x1818
    dw 0x3c00

// 0011 1100
// 0011 0110
// 0011 1100
// 0011 0000
// 0011 0000
P_SPRITE:
    dw 0x3c36
    dw 0x3c30
    dw 0x3000

// 0000 0000
// 0000 0000
// 0111 1110
// 0000 0000
// 0000 0000
MINUS_SPRITE:
    dw 0x0000
    dw 0x7e00
    dw 0x0000

DOT_SPRITE:
    dw 0x1000

main:
    ld v0, 9
    ld v1, 7
    ld I, [C_SPRITE]
    drw v0, v1, 5

    ld v0, 0x11 ; literals can be hexadecimal, decimal or octal
    ld v1, 7
    ld I, [h_sprite] ; all text is case insensitive
    drw v0, v1, 5

    ld v0, 24
    ld v1, 7
    ld I, [I_sprite]
    drw v0, v1, 5

    ld v0, 30
    ld v1, 7
    ld I, [P_sPrItE]
    drw v0, v1, 5

    ld v0, 38
    ld v1, 7
    ld I, [minus_sprite]
    drw v0, v1, 5

    ld v0, 057 ; octal
    ld v1, 7
    ld vC, 8  // register's number are hexadecimal without '0x'
    ld F, vC
    drw v0, v1, 5

    jp [loop] -- it's unnecessary here

loop:
    ld  v4, k
    sne v4, 1
    jp [drw1]
    sne v4, 2
    jp [drw2]
    sne v4, 3
    jp [drw3]
    sne v4, 0xC
    jp [drw4]

    jp [endlessloop]

drw1:
    ld v0, 18
    ld v1, $SECOND_OFFSET // $ operator returns the value of mark (the dw command)
    ld I, [C_SPRITE]
    drw v0, v1, 5
    jp [loop]

drw2:
    ld v0, 25
    ld v1, $SECOND_OFFSET
    ld I, [H_SPRITE]
    drw v0, v1, 5
    jp [loop]

drw3:
    ld v0, 31
    ld v1, $SECOND_OFFSET
    ld I, [I_SPRITE]
    drw v0, v1, 5
    jp [loop]

drw4:
    ld v0, 36
    ld v1, $SECOND_OFFSET
    ld I, [P_SPRITE]
    drw v0, v1, 5
    jp [loop]

endlessLOOP:
    jp [endlessLOOP]

SECOND_OFFSET: // the marks can be everywhere, even if mark declared after it was used
    dw 0x0013 // the marks calculated into pairs<address, value>; value is a dw command
// Those marks' values are generated at compile time