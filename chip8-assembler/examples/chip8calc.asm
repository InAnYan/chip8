ld v7, 0
ld v9, 255
ld vc, 1

mainLoop:
	ld   v0, $INPUT_KEY
	call [printChar]
bad:
	ld   v0, K
	call [specialPrintchar]
	sne  vf, 1
	jp   [bad]
    ld   I, [inputbuffer]
	add  I, v7
	add  v7, 1
	ld   [i], v0 
	
	subLoop:
		ld   v0, K
		sne  v0, $unprintKey
		jp [subLoopUnprint]
		se   v0, $new_line_key
		call [printChar]

		ld   I, [inputbuffer]
		add  I, v7
		add  v7, 1
		ld   [i], v0 
		
		sne  v7, 3
		jp   [doSomeWork]
		jp   [subloop] ; case insensitive

	subLoopUnprint:
		call [unprintchar]
		sub v7, vc
		jp [subloop]

doSomeWork:
	ld  v0, $new_line_key
	call [printchar]

	ld  I, [inputbuFFer]
	ld  v3, [I]

	sne v0, 1
	call [doADD]
	sne v0, 2
	call [doSub]
	sne v0, 3
	call [doMult]
	sne v0, 4
	call [doDIV]
	
	ld  I, [inputbuFfer]
	ld  b, v4
	ld  v2, [I]

	ld  v4, v0
	ld  v5, v1
	ld  v6, v2

	ld  v0, v4
	call [printchar]
	ld  v0, v5
	call [printchar]
	ld  v0, v6
	call [printchar]
	ld  v0, $new_line_key
	call [printchar]

	ld  v0, 0xFF
	ld  v1, 0xfF
	ld  v2, 0xFf
	ld  v3, 0xff
	ld  I, [inputbuFfer]
	ld  [i], v3
	ld  v7, 0

	jp  [mainloop]

doAdd:
	ld v4, v1
	add v4, v2
	ret

doSUB:
	ld  v4, v1
	sub v4, v2
	
	se vf, 0
	ret
	
	ld  v0, $MINUS_KEY
	call [printChar]
	sub v9, v4
	ld  v4, v9
	add v4, 1
	ld  v9, 255
	ret

doMult:
	ld  v4, 0
	ld  v5, v2
ll:	add v4, v1
	sub v5, vc
	sne v5, 0
	ret
	jp  [ll]

doDIV:
	ld  v5, v1
	ld  v4, 0
sbd: sub v5, v2
	add v4, 1
	se  vf, 1
	ret
	jp [sbd]

// useless, because supports only internal CHIP-8 font
// v0 - char argument. function is unsafe for registers
printchar: // case insensitive	
	ld  v2, v0  // saving argument

	ld  I, [printCharPos]
	ld  v1, [I] // loading printing position
	
	sne v2, $NEW_LINE_KEY
	jp [newLine]
	jp [defaultPrintChar]

	defaultPrintChar:
		sne v2, $MINUS_KEY
		jp [minkeyprint]

		se  v2, $INPUT_KEY
		ld  F,  v2   // getting sprite representation of argument
		sne v2, $INPUT_KEY
		ld  I, [INPUT_SYMBOL_SPRITE]
		jp [proceedPrint]
	minKeyPrint: ld  I, [MINUS_SYMBOL_SPRITE]
	proceedPrint:
		sne v0, 0x1f  // x pos greater than half of screen width
		add v1, 6     // incrementing y pos
		sne v0, 0x1f
		ld  v0, 1     // resetting x pos

		sne v1, 31    // y pos greater than screen height
		ld  v0, 0x1f  // setting new x pos
		sne v1, 31
		ld  v1, 1     // resetting y pos

		sne v0, 0x3d  // x pos greater than screen width
		add v1, 6     // incrementing y pos
		sne v0, 0x3d
		ld  v0, 0x1f  // resetting x pos

		sne v1, 0x1f  // y big
		cls
		sne v1, 0x1f
		ld  v0, 1
		sne v1, 0x1f
		ld  v1, 1

		drw v0, v1, 5 // drawing !
	
		add v0, 5     // incrementing x pos	
		jp [printcharend]

	newLine:
		ld  v4, v0
		ld  vc, 0x20
		sub v4, vc
		ld  vc, 1
		se  vf, 1
		ld  v0, 1
		sne vf, 1
		ld  v0, 0x3d

		se  v0, 0x3d
		add v1, 6
		jp [printcharend]

	printCharEnd:
		ld I, [printCharPos]
		ld [I], v1  // saving printing position
		ld v0, v2   // restoring argument
		ld I, [lastKey]
		ld [I], v0  // saving last key for unprint function
		ret

specialPrintChar:
	ld  v2, v0  // saving argument
	ld  I, [printCharPos]
	ld  v1, [I] // loading printing position
	sne v2, 0x01
	jp  [plusprint]
	sne v2, 0x02
	jp  [minusprint]
	sne v2, 0x03
	jp  [multipPrint]
	sne v2, 0x04
	jp  [divPrint]
	ld  vf, 1
	ret

plusPrint:  ld I, [PLUS_SYMBOL_SPRITE]
		    jp   [proceedPrint]
minusPrint: ld I, [MINUS_SYMBOL_SPRITE]
		    jp   [proceedPrint]
multipPrint: ld I, [MULTIP_SYMBOL_SPRITE]
		    jp   [proceedPrint]
divPrint: ld I, [DIV_SYMBOL_SPRITE]
		    jp   [proceedPrint]

unPrintChar:
	ld  vb, v0

	ld  I,  [printCharPos]
	ld  v1, [I]
	
	ld  vA, 5
	sub v0, vA
	
	ld  I, [printCharPos]
	ld  [I], v1

	ld  I, [lastKey]
	ld  v0, [I]
	call [printChar]

	ld  I,  [printCharPos]
	ld  v1, [I]
	
	ld  vA, 5
	sub v0, vA
	
	ld  I, [printCharPos]
	ld  [I], v1

	ld  v0, vb
	ret

endlessLoop:
	jp [endlessLoop]

printCharPos: // like a variable
	dw 0x0101

NEW_LINE_KEY: // all "variables" should be two bytes long
	dw 0x000A

INPUT_KEY:	
	dw 0x0010

// 1000 0000
// 0100 0000
// 0010 0000
// 0100 0000
// 1000 0000

INPUT_SYMBOL_SPRITE:	
	dw 0x8040
	dw 0x2040
	dw 0x8000

// 0000 0000
// 0000 0000
// 1111 0000
// 0000 0000
// 0000 0000

MINUS_SYMBOL_SPRITE:
	dw 0x0000
	dw 0xF000
	dw 0x0000

MINUS_KEY:
	dw 0x0015

inputBuffer:
	dw 0x0000
	dw 0x0000
	dw 0x0000
	dw 0x0000

bcdPrintBUffer:
	dw 0x0000
	dw 0x0000

unprintKey:
	dw 0x0000

lastKey:
	dw 0x0000

// 0000 0000
// 0100 0000
// 1100 0000
// 0100 0000
// 0000 0000
PLUS_SYMBOL_SPRITE:
	dw 0x0040
	dw 0xe040
	dw 0x0000

// 0000 0000
// 1001 0000
// 0110 0000
// 0110 0000
// 1001 0000
MULTIP_SYMBOL_SPRITE:
	dw 0x0090
	dw 0x6060
	dw 0x9000

// 0001 0000
// 0010 0000
// 0110 0000
// 0100 0000
// 1000 0000
DIV_SYMBOL_SPRITE:
	dw 0x1020
	dw 0x6040
	dw 0x8000