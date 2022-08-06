#include <iostream>
#include <string>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <fstream>

#include "newc8asm.hpp"

#define ARGS_FIND(args, cmd) ((argIndex = find((args).begin(), (args).end(), cmd) - args.begin()) != (args).size())

using namespace c8asm;
using namespace std;

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
		cout << "chip8-new-assembler v0.5 - CHIP-8 assembler" << endl
			 << "Copyright(C) 2022 Ruslan Popov <ruslanpopov1512@gmail.com>" << endl << endl;
	}
	
	bool asmFound = ARGS_FIND(args, "-a") || ARGS_FIND(args, "--assemble");
	size_t asmFileIndex = argIndex + 1;
	bool outFound = ARGS_FIND(args, "-o") || ARGS_FIND(args, "--output");
	size_t outFileIndex = argIndex + 1;
	if (asmFileIndex >= args.size() && asmFound || outFileIndex >= args.size() && outFound)
	{
		cout << "ERROR: No files specified" << endl;
		return 1;
	}

	if (asmFound)
	{
		ifstream input(args[asmFileIndex], ios::in | ios::binary);
		ofstream output(outFound ? args[outFileIndex] : (args[asmFileIndex] + ".ch8"), ios::out | ios::binary);
		if (input.fail() || output.fail())
		{
			cout << "ERROR: Can't open files" << endl;
			return 1;
		}

	    vector<string> file;
		while (!input.eof())
		{
			string buf;
			getline(input, buf);
			file.push_back(buf);
		}

		bool noError = true;
		try
		{
		    tokens tokenList = tokenize(file);
			generator gen(tokenList);
			opcodes_program opcodesList = gen.generateBytes();

			for (size_t i = 0; i < opcodesList->size(); i++)
			{
				uint16_t data = (*opcodesList)[i];
				uint8_t b1 = data >> 8;
				uint8_t b2 = data & 0x00FF;
				output.write(reinterpret_cast<const char*>(&b1), 1);
				output.write(reinterpret_cast<const char*>(&b2), 1);
			}
		}
		catch (generator_exception const& e)
		{
			cout << e.what() << endl;
			cout << "    [" << e.where().first << "]: " << strtrim(file.at(e.where().first)) << endl;
			cout << string(8 + to_string(e.where().first).size(), ' ') << string(e.where().second, ' ') << '^' << endl;
			noError = false;
		}

		input.close();
		output.close();
		if (!noSplashFound && noError) cout << "Done." << endl;
		return 0;
	}
	else
	{
		cout << "ERROR: No command specified: \"-a\"" << endl;
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
