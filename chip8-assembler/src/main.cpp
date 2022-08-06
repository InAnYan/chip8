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
#include "disassembler.hpp"
#include "assembler.hpp"

#define ARGS_FIND(args, cmd) ((argIndex = find((args).begin(), (args).end(), cmd) - args.begin()) != (args).size())

using namespace assembler;

string strtrim(string str);

int main(int argc, char** argv)
{
	vector<string> args;
	size_t argIndex; // Macros requirement
	for (int i = 0; i < argc; i++)
	{
		args.push_back(argv[i]);
	}

	bool noSplashFound = ARGS_FIND(args, "-ns") || ARGS_FIND(args, "--no-splash");
	if (!noSplashFound)
	{
		cout << "chip8-assembler v0.5 - CHIP-8 assembler and disassembler." << endl
			 << "Copyright(C) 2021, 2022 Ruslan Popov <ruslanpopov1512@gmail.com>" << endl << endl;
	}

	bool disasmFound = ARGS_FIND(args, "-d") || ARGS_FIND(args, "--disassemble");
	size_t disasmFileIndex = argIndex + 1;
	bool asmFound = ARGS_FIND(args, "-a") || ARGS_FIND(args, "--assemble");
	size_t asmFileIndex = argIndex + 1;
	bool outFound = ARGS_FIND(args, "-o") || ARGS_FIND(args, "--output");
	size_t outFileIndex = argIndex + 1;
	if (disasmFound && asmFound)
	{
		cout << "ERROR: Only one operation at once" << endl;
		return 1;
	}
	if (disasmFileIndex >= args.size() && disasmFound || asmFileIndex >= args.size() && asmFound || outFileIndex >= args.size() && outFound)
	{
		cout << "ERROR: No files specified" << endl;
		return 1;
	}

	if (disasmFound)
	{
		ifstream input(args[disasmFileIndex], ios::in | ios::binary);
		ofstream output(outFound ? args[outFileIndex] : (args[disasmFileIndex]+".asm"), ios::out | ios::binary);
		if (input.fail() || output.fail())
		{
			cout << "ERROR: Can't open files" << endl;
			return 1;
		}

		while (!input.eof())
		{
			char byte1;
			char byte2;
			input.read(&byte1, 1);
			input.read(&byte2, 1);
			output << assembler::disasmCode((byte1 & 0xFF) * 0x100 + (byte2 & 0xFF)) << endl;
		}

		input.close();
		output.close();
		if (!noSplashFound) cout << "Done." << endl;
		return 0;
	}
	else if (asmFound)
	{
		ifstream input(args[asmFileIndex], ios::in | ios::binary);
		ofstream output(outFound ? args[outFileIndex] : (args[asmFileIndex] + ".ch8"), ios::out | ios::binary);
		if (input.fail() || output.fail())
		{
			cout << "ERROR: Can't open files" << endl;
			return 1;
		}

		shared_ptr<vector<string>> file = make_shared<vector<string>>();
		while (!input.eof())
		{
			string buf;
			getline(input, buf);
			file->push_back(buf);
		}

		bool noError = true;
		try
		{
			shared_ptr<vector<token_t>> tokens = assembler::tokenize(file);
			assembler::context_t scope;
			assembler::parser mainParser(tokens, scope);
			assembler::program mainProgram = move(mainParser.parse());

			for (size_t i = 0; i < mainProgram->size(); i++)
			{
				dbyte data = (*mainProgram)[i]->codegen(scope);
				byte b1 = data >> 8;
				byte b2 = data & 0x00FF;
				output.write(reinterpret_cast<const char*>(&b1), 1);
				output.write(reinterpret_cast<const char*>(&b2), 1);
			}
		}
		catch (assembler_exception const& e)
		{
			cout << e.what() << endl;
			cout << "    [" << e.where().first << "]: " << strtrim(file.get()->at(e.where().first - 1)) << endl;
			cout << string(8 + to_string(e.where().first).size(), ' ') << string(e.where().second-1, ' ') << '^' << endl;
			noError = false;
		}

		input.close();
		output.close();
		if (!noSplashFound && noError) cout << "Done." << endl;
		return 0;
	}
	else
	{
		cout << "ERROR: No command specified (\"-d\" or \"-a\")" << endl;
		return 1;
	}

	return 0;
}

string strtrim(string str)
{
	stringstream buf;
	size_t i = 0;
	while (str[i] == '\n' || str[i] == '\r' || str[i] == '\t') i++;
	for (; i < str.size(); i++)
		if (str[i] == '\t')
			buf << ' ';
		else
			buf << str[i];

	return buf.str();
}