/*
chip8-assembler v0.5 - CHIP-8 assembler and disassembler.
Copyright(C) 2021, 2022 Ruslan Popov <ruslanpopov1512@gmail.com>

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "program.hpp"

namespace assembler
{
	string disasmCode(int code)
	{
		// Foolproof
		if (code > 0xFFFF) throw invalid_argument("The code is bigger than 2 bytes");

		stringstream res;

		if (code <= 0xFF)
		{
			if (code == 0xE0)
			{
				return "cls";
			}
			else if (code == 0xEE)
			{
				return "ret";
			}
			else
			{
				res << "db 0x" << setw(4) << setfill('0') << hex << code;
				return res.str();
			}
		}

		switch (code >> 12)
		{
		case 0x1:
			res << "jp 0x" << hex << (code & 0x0FFF);
			break;
		case 0x2:
			res << "call 0x" << hex << (code & 0x0FFF);
			break;
		case 0x3:
			res << "se V" << hex << ((code >> 8) & 0x0F) << ", 0x" << (code & 0x00FF);
			break;
		case 0x4:
			res << "sne V" << hex << ((code >> 8) & 0x0F) << ", 0x" << (code & 0x00FF);
			break;
		case 0x5:
			if ((code & 0x000F) == 0)
				res << "se V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
			else
				res << "dw 0x" << hex << code;
			break;
		case 0x6:
			res << "ld V" << hex << ((code >> 8) & 0x0F) << ", 0x" << (code & 0x00FF);
			break;
		case 0x7:
			res << "add V" << hex << ((code >> 8) & 0x0F) << ", 0x" << (code & 0x00FF);
			break;
		case 0x8:
			switch (code & 0x000F)
			{
			case 0x0:
				res << "ld V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
				break;
			case 0x1:
				res << "or V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
				break;
			case 0x2:
				res << "and V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
				break;
			case 0x3:
				res << "xor V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
				break;
			case 0x4:
				res << "add V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
				break;
			case 0x5:
				res << "sub V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
				break;
			case 0x6:
				res << "shr V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0x7:
				res << "subn V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4);
				break;
			case 0xE:
				res << "shl V" << ((code >> 8) & 0x0F);
				break;
			default:
				res << "db 0x" << setw(4) << setfill('0') << hex << code;
				break;
			}
			break;
		case 0x9:
			res << "sne V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code >> 4) & 0x00F);
			break;
		case 0xA:
			res << "ld I, " << hex << "0x" << (code & 0x0FFF);
			break;
		case 0xB:
			res << "jp V0, " << hex << "0x" << (code & 0xFFF);
			break;
		case 0xC:
			res << "rnd V" << hex << ((code >> 8) & 0x0F) << ", 0x" << (code & 0x00FF);
			break;
		case 0xD:
			res << "drw V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4) << ", 0x" << (code & 0x000F);
			break;
		case 0xE:
			switch (code & 0x00FF)
			{
			case 0x9E:
				res << "skp V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0xA1:
				res << "sknp V" << hex << ((code >> 8) & 0x0F);
				break;
			default:
				res << "db 0x" << setw(4) << setfill('0') << hex << code;
				break;
			}
			break;
		case 0xF:
			switch (code & 0x00FF)
			{
			case 0x07:
				res << "ld V" << hex << ((code >> 8) & 0x0F) << ", DT";
				break;
			case 0x0A:
				res << "ld V" << hex << ((code >> 8) & 0x0F) << ", K";
				break;
			case 0x15:
				res << "ld DT, V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0x18:
				res << "ld ST, V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0x1E:
				res << "add I, V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0x29:
				res << "ld F, V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0x33:
				res << "ld B, V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0x55:
				res << "ld [I], V" << hex << ((code >> 8) & 0x0F);
				break;
			case 0x65:
				res << "ld V" << hex << ((code >> 8) & 0x0F) << ", [I]";
				break;
			default:
				res << "db 0x" << setw(4) << setfill('0') << hex << code;
				break;
			}
			break;
		default:
			res << "db 0x" << hex << code;
			break;
		}

		return res.str();
	}
}