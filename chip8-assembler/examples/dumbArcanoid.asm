// Dumb arcanoid [Ruslan Popov, 2022]
// dumb because game plays itself
// 1 and 3 - default controls

// Start pos
ld v0, 20
ld v1, 20
// Start vector
ld v2, 1
ld v3, 1
// Plate pos
ld v6, $PLATE_X
ld v7, $PLATE_Y
// Keys
ld v8, 1
ld v9, 3
// Substraction register
ld vA, 1
// Pads
ld vB, 0
ld vC, $PADS_START
ld vD, 0xF8 ; 1111 1000
// Pads draw
ld I, [PAD_SPRITE]
drawPadLoop:
    drw vB, vC, 1
    add vB, 8
    sne vB, $SCREEN_X_FOR_PADS
    call [drawPadTemp]
    sne vC, $PADS_END
    jp [cont2]
    jp [drawPadLoop]
drawPadTemp:
    add vC, 2
    ld vB, 0
    ret

cont2: 
ld I, [PLATE_SPRITE]
drw v6, v7, 1
ld I, [DOT_SPRITE]

mainLoop:
    // v0, v1 - new dot pos
    // v2, v3 - dot vector
    // v4, v5 - old dot pos
    // v6, v7 - plate pos
    // v8, v9 - for keys
    // vA     - substract 1
    // vB, vC - pad pos
    // vD     - pad AND

    // Drawing
    ld I, [DOT_SPRITE]
    drw v0, v1, 1
    // Saving old pos
    ld v4, v0
    ld v5, v1

    // Collision check and vector changing
    // Corner problem (maybe)
    sne vf, 1
    call [dotCollision]
    // Left side
    sne v0, 0
    ld v2, 1
    // Right side
    sne v0, $SCREEN_X
    ld v2, 0xff
    // Up
    sne v1, 0
    ld v3, 1
    // Bottom
    sne v1, $SCREEN_Y
    ld v3, 0xff
    add v0, v2
    add v1, v3
cont:
    drw v4, v5, 1 ; erasing dot
    ld I, [PLATE_SPRITE]
    drw v6, v7, 1
    sknp v8
    sub v6, vA
    sknp v9
    add v6, 1
    drw v6, v7, 1
    jp [mainLoop]

dotCollision:
    sne v3, 1
    ld v3, 0xfd
    add v3, 2
    se v1, $PLATE_Y ; collision with plate
    jp [blockCollision]
    ret

blockCollision:
    ld vB, v0
    ld vC, v1
    drw v0, v1, 1
    and vB, vD ; 1111 1000
    ld I, [PLATE_SPRITE]
    drw vB, vC, 1
    ld I, [PLATE_SPRITE_2]
    drw vB, vC, 1
    ld I, [DOT_SPRITE]
    drw v0, v1, 1
    ret

// 1000 0001
PLATE_SPRITE_2:
    dw 0x8100

// 1000 0000
DOT_SPRITE:
    dw 0x8000

// 1111 1111
PLATE_SPRITE:
    dw 0xFF00

SCREEN_X: ; you can change this for Super CHIP-8
    dw 63

SCREEN_X_FOR_PADS: ; unfortunately, can't use ariphmetic operations for constant values
    dw 0x40

SCREEN_Y: ; you can change this for Super CHIP-8
    dw 31

PLATE_Y:
    dw 28

PLATE_X:
    dw 23

// 0111 1110
PAD_SPRITE:
    dw 0x7E00

PADS_START:
    dw 5

PADS_END:
    dw 15