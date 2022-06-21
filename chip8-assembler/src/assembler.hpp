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

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "program.hpp"

namespace assembler
{
	enum token_type_t
	{
		COMMAND,
		REGISTER,
		VALUE,
		MARK,
		DATA,
		ADDRESS,
		OTHER,

		NONE // For parser
	};
	typedef token_type_t const& token_type;
	inline string token_type_to_string(token_type type);

	typedef char byte;
	typedef short dbyte;

	struct token_t
	{
		token_type_t type;
		string str;
		size_t line;
		size_t pos;
		token_t() : type(NONE), str(""), pos(0), line(0) {}
		token_t(token_type type, string str, size_t pos, size_t line) : type(type), str(str), pos(pos), line(line) {}
	};
	typedef token_t const& token;

	shared_ptr<vector<token_t>> tokenize(shared_ptr<vector<string>> const& file);

	typedef map<string, pair<dbyte, dbyte>> context_t; // First is addres, second is value
	typedef context_t const& context;

	class expression
	{
	protected:
		token cmd;
		token arg1;
		token arg2;
		token arg3;
	public:
		expression(token cmd) : cmd(cmd), arg1(token_t()), arg2(token_t()), arg3(token_t()) {}
		expression(token cmd, token arg1) : cmd(cmd), arg1(arg1), arg2(token_t()), arg3(token_t()) {}
		expression(token cmd, token arg1, token arg2) : cmd(cmd), arg1(arg1), arg2(arg2), arg3(token_t()) {}
		expression(token cmd, token arg1, token arg2, token arg3) : cmd(cmd), arg1(arg1), arg2(arg2), arg3(arg3) {}

		virtual int codegen(context scope);
		virtual int size() const { return 2; };

		token getCommand() const { return cmd; }
	};

	typedef unique_ptr<vector<unique_ptr<expression>>> program;
	class parser
	{
	private:
		size_t parserPos;
		size_t parserCodePos;
		size_t parserLinePos;
		shared_ptr<vector<token_t>> tokens;
		context_t& scope;

		token parserMatch(token_type expected);
		token parserRequire(token_type expected);
		token parserRequire();
		token parserNext();

		unique_ptr<expression> parseCmd();
	public:
		parser(shared_ptr<vector<token_t>> tokens, context_t& scope) : tokens(tokens), scope(scope),
			parserPos(0), parserCodePos(0), parserLinePos(0) {}

		program parse();

		static int parseValue(token t, context scope, int max);
		static int parseReg(token t);
	};

	class assembler_exception : public std::exception
	{
	private:
		string msg;
		size_t line;
		size_t pos;
	public:
		assembler_exception(string msg, size_t line, size_t pos) : msg(msg), line(line), pos(pos) {}

		const char* what() const throw() 
		{ 
			string res = "ERROR: " + msg + " on line " + to_string(line) + " on pos " + to_string(pos);
			return _strdup(res.c_str());
		}
		pair<size_t, size_t> where() const throw() { return {line, pos}; }
	};
}
#endif
