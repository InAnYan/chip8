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

#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#define WHITE IM_COL32(255, 255, 255, 255)
#define BLACK IM_COL32(0, 0, 0, 255)
#define RED IM_COL32(234, 5, 5, 255)
#define BLUE IM_COL32(30, 30, 234, 255)
#define YELLOW IM_COL32(234, 183, 0, 255)
#define GREEN IM_COL32(0, 230, 0, 255)

inline int random(int min, int max)
{
    static bool first = true;
    if (first)
    {
        srand((unsigned int)time(NULL));
        first = false;
    }
    return min + rand() % ((max + 1) - min);
}

template<typename T>
inline std::string type_to_hex(T i)
{
    std::stringstream stream;
    stream << "0x"
        << std::setfill('0') << std::setw(sizeof(T) * 2)
        << std::hex << i;
    return stream.str();
}

#endif