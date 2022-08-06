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

#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <array>
#include <fstream>
#include <functional>

class CHIP8
{
public:
	using byte = unsigned char;
	using dbyte = uint16_t;

private:
	std::string romPath;
	std::ifstream romFile;

	std::array<std::array<bool, 64>, 32> graphicsMap;
	std::array<byte, 4096> ram;
	std::array<dbyte, 16> stack;
	std::array<bool, 16> keys;

	byte sp;
	std::array<byte, 16> v;
	dbyte I, pc, code;
	
	bool endlessLoop;

	bool drawAlgorithm(byte vx, byte vy, byte n);

	// Used for logging
	void errorInternal(std::string const& text);
	void infoInternal(std::string const& text);

public:
	CHIP8();
	CHIP8(std::string currentPath);
	~CHIP8();
	
	bool reload(std::string newPath);
	void refresh();
	void emulateCycle();

	void setKey(int key) { keys[key] = true; lastKey = key; }
	void unsetKey(int key) { keys[key] = false; }
	void setRam(dbyte addr, byte value) { ram[addr] = value; }
	void resetEndlessLoop() { endlessLoop = false; }

	byte soundTimer, delayTimer; // Exception
	int lastKey;
	std::function<void(std::string const&)> logCallback;

	bool display(int x, int y) const { return graphicsMap[y][x]; }
	bool caughtEndlessLoop() const { return endlessLoop; }
	std::string regInfo() const;
	dbyte getPC() const { return pc; }
	dbyte getI() const { return I; }
	byte getFromRam(dbyte addr) const { return ram[addr]; }

	static std::string disasmCode(int code);
};

#endif