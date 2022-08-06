/*
chip8-emulator v1.0 - CHIP-8 emulator.
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

#include "CHIP8.hpp"
#include "../common.h"
#include "../log/logger.hpp"

using namespace std;

unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

string CHIP8::disasmCode(int code)
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
            res << "dw 0x" << hex << code;
            return res.str();
        }
    }

    switch (code >> 12)
    {
    case 0x1:
        res << "jmp 0x" << hex << (code & 0x0FFF);
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
        if((code & 0x000F) == 0)
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
            res << "dw 0x" << hex << code;
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
        res << "jp V0, " << hex << "0x" << (code & 0x0FF);
        break;
    case 0xC:
        res << "rnd V" << hex << ((code >> 8) & 0x0F) << ", 0x" << (code & 0x00FF);
        break;
    case 0xD:
        res << "drw V" << hex << ((code >> 8) & 0x0F) << ", V" << ((code & 0x00FF) >> 4) << ", " << (code & 0x000F);
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
            res << "dw 0x" << hex << code;
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
            res << "dw 0x" << hex << code;
            break;
        }
        break;
    default:
        res << "dw 0x" << hex << code;
        break;
    }

    return res.str();
}

CHIP8::CHIP8()
{
    logger::debug("Initializing CHIP-8...");

	refresh();

    fill(ram.begin(), ram.end(), 0);
    for (int i = 0; i < 80; i++)
    {
        ram[i] = chip8_fontset[i];
    }
}

bool CHIP8::drawAlgorithm(byte vx, byte vy, byte n)
{
    // Foolproof
    if (I + n > 0xFFF)
    {
        errorInternal("Segmentation fault: I + n > 0xFFF");
        return false;
    }

    bool collision = false;
    for (int i = 0; i < n; i++)
    {
        byte state = ram[I + i];
        for (int j = 0; j < 8; j++)
        {
            if (state & (0b10000000 >> j))
            {
                collision = graphicsMap[(vy + i) % 32][(vx + j) % 64];
                graphicsMap[(vy + i) % 32][(vx + j) % 64] ^= 1;
            }
        }
    }
    return collision;
}

CHIP8::CHIP8(string currentPath)
{
    fill(ram.begin(), ram.end(), 0);
    for (int i = 0; i < 80; i++)
    {
        ram[i] = chip8_fontset[i];
    }

	reload(currentPath);
}

bool CHIP8::reload(string newPath)
{
    logger::debug("Reloading CHIP-8...");

    refresh();
    romPath = newPath;
    romFile.close();
    
    romFile.open(romPath, ios::in | ios::binary);
    if (!romFile)
    {
        // TODO: Error handle
        return false;
    }
    romFile.seekg(0, romFile.end);
    long long length = romFile.tellg();
    romFile.seekg(0, romFile.beg);
    if (length > 3584 || length == -1)
    {
        // TODO: Error handle
        return false;
    }

    char b = 0;
    for (dbyte i = 0x200; romFile.get(b); i++)
        ram[i] = b;

    return true;
}

void CHIP8::refresh()
{
    logger::debug("CHIP-8 refresh");
    for (int i = 0; i < 32; i++)
    {
        fill(graphicsMap[i].begin(), graphicsMap[i].end(), false);
    }
	fill(stack.begin(), stack.end(), 0);
	fill(keys.begin(), keys.end(), false);
	fill(v.begin(), v.end(), 0);

	sp = 0;
	soundTimer = 0;
	delayTimer = 0;
	I = 0;
	pc = 0x200;
	code = 0;
    lastKey = -1;

    endlessLoop = false;
}

CHIP8::~CHIP8()
{
    romFile.close();
}

void CHIP8::emulateCycle()
{
    if (endlessLoop) return;

    code = ram[pc] << 8 | ram[pc + 1];
    pc += 2;
    if (code == 0xE0) // CLS
    {
        for (int i = 0; i < 32; i++)
        {
            fill(graphicsMap[i].begin(), graphicsMap[i].end(), false);
        }
        return;
    }
    if (code == 0xEE)
    {
        pc = stack[sp];
        sp--;
        return;
    }

    byte nn = code & 0x00FF;
    dbyte addr = code & 0x0FFF;
    byte x = (code >> 8) & 0x0F;
    byte y = (code >> 4) & 0x0F;
    byte nibble = code & 0x000F;

    switch (code >> 12)
    {
    case 0x1: // JP addr
        pc = addr;
        if ((ram[pc] << 8 | ram[pc + 1]) == code)
        {
            endlessLoop = true;
            infoInternal("Caught endless loop");
        }
        break;
    case 0x2: // CALL addr
        sp++;
        if (sp >= 16)
        {
            errorInternal("Stack overflow");
        }
        if (sp < 16) stack[sp] = pc;
        pc = addr;
        if ((ram[pc] << 8 | ram[pc + 1]) == code)
        {
            endlessLoop = true;
            infoInternal("Caught endless loop");
        }
        break;
    case 0x3: // SE Vx, byte
        if (v[x] == nn)
        {
            pc += 2;
        }
        break;
    case 0x4: // SNE Vx, byte
        if (v[x] != nn)
        {
            pc += 2;
        }
        break;
    case 0x5: // SE Vx, Vy
        if ((v[x] == v[y]))
        {
            pc += 2;
        }
        break;
    case 0x6: // LD Vx, byte 
        v[x] = nn;
        //if (x == 0xf) logger::error("F:" + to_string(nn));
        break;
    case 0x7: // ADD Vx, byte
        v[x] += nn;
        break;
    case 0x8:
        switch (code & 0x000F)
        {
        case 0x0: // LD Vx, Vy
            v[x] = v[y];
            break;
        case 0x1: // OR Vx, Vy
            v[x] = v[x] | v[y];
            break;
        case 0x2: // AND Vx, Vy
            v[x] = v[x] & v[y];
            break;
        case 0x3: // XOR Vx, Vy
            v[x] = v[x] ^ v[y];
            break;
        case 0x4: // ADD Vx, Vy
            v[0xF] = (v[x] + v[y]) > 0xFF;
            v[x] += v[y];
            break;
        case 0x5: // SUB Vx, Vy
            v[0xF] = v[x] > v[y];
            v[x] -= v[y];
            break;
        case 0x6: // SHR Vx 
            if (v[x] & 0b00000001) v[0xF] = 1;
            else v[0xF] = 0;
            v[x] = v[x] >> 1;
            break;
        case 0x7: // SUBN Vx, Vy
            v[0xF] = v[y] > v[x];
            v[x] = v[y] - v[x];
            break;
        case 0xE: // SHL Vx
            if (v[x] & 0b10000000) v[0xF] = 1;
            else v[0xF] = 0;
            v[x] <<= 1;
            break;
        default:
            errorInternal("Unknown opcode: " + type_to_hex(code));
            break;
        }
        break;
    case 0x9: // SNE Vx, Vy
        if (v[x] != v[y])
        {
            pc += 2;
        }
        break;
    case 0xA: // LD I, addr
        I = addr;
        break;
    case 0xB: // JP V0, addr
        pc = addr + v[0];
    case 0xC: // RND Vx, byte
        v[x] = random(0x0, 0xFF) & nn;
        break;
    case 0xD: // DRW Vx, Vy, nibble
        v[0xF] = drawAlgorithm(v[x], v[y], nibble);
        break;
    case 0xE:
        switch (code & 0x00FF)
        {
        case 0x9E: // SKP Vx
            if (keys[v[x]] == true)
            {
                pc += 2;
                
            }
            break;
        case 0xA1: // SKNP Vx
            if (keys[v[x]] != true)
            {
                pc += 2;
            }
            break;
        default:
            errorInternal("Unknown opcode: " + type_to_hex(code));
            break;
        }
        break;
    case 0xF:
        switch (code & 0x00FF)
        {
        case 0x07: // LD Vx, DT
            v[x] = delayTimer;
            break;
        case 0x0A: // LD Vx, K
            if (lastKey >= 0)
            {
                v[x] = lastKey;
                lastKey = -1;
            }
            else pc -= 2;
            break;
        case 0x15: // LD DT, Vx
            delayTimer = v[x];
            break;
        case 0x18: // LD ST, Vx
            soundTimer = v[x];
            break;
        case 0x1E: // ADD I, Vx
            I = I + v[x];
            break;
        case 0x29: // LD F, Vx
            I = v[x] * 5;
            break;
        case 0x33:
            if (I >= 0xFFF) errorInternal("Segmentation fault: I >= 0xFFF");
            ram[I] = (v[x] / 100) % 10;
            ram[I + 1] = (v[x] / 10) % 10;
            ram[I + 2] = v[x] % 10;
            break;
        case 0x55:
            if (I >= 0xFFF && x != 0) errorInternal("Segmentation fault: I >= 0xFFF");
            for (int i = 0; i <= x; i++)
            {
                ram[I + i] = v[i];
            }
            break;
        case 0x65:
            if (I >= 0xFFF && x != 0) errorInternal("Segmentation fault: I >= 0xFFF");
            for (int i = 0; i <= x; i++)
            {
                v[i] = ram[I + i];
            }
            break;
        default:
            errorInternal("Unknown opcode: " + type_to_hex(code));
            break;
        }
        break;
    default:
        errorInternal("Unknown opcode: " + type_to_hex(code));
        break;
    }
}

string CHIP8::regInfo() const
{
    stringstream buf;
    buf << hex;
    for (byte i = 1; i <= 16; i++)
    {
        buf << "V" << noshowbase << i - 1 << " = " << showbase << setw(4) << (int)v[i - 1] << "; ";
        if (i % 3 == 0) buf << endl;
    }
    buf << endl << "\nI  = " << setw(4) << setfill('0') << I << "; PC = " << setw(4) << setfill('0') << pc << ";" << endl << "ST =  " << setw(4) << (int)soundTimer << "; DT =  " << setw(4) << (int)delayTimer << "; SP = " << (int)sp << endl;
    return buf.str();
}

void CHIP8::errorInternal(string const& text)
{
    logger::error("ERROR: " + text);
    if (logCallback) logCallback("ERROR: " + text);
    endlessLoop = true;
}

void CHIP8::infoInternal(string const& text)
{
    logger::info("INFO: " + text);
    if (logCallback) logCallback("INFO: " + text);
}