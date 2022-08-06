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

#include "assembler.hpp"

using namespace assembler;

static set<string> keywords = {"cls", "ret", "ld", "and", "or", "xor", "call", 
							   "se", "sne", "add", "sub", "shr", "shl", "subn",
							   "dw", "jp", "rnd", "drw", "skp", "sknp"};

inline string assembler::token_type_to_string(token_type type)
{
	switch (type)
	{
	case COMMAND:
		return "COMMAND";
	case REGISTER:
		return "REGISTER";
	case VALUE:
		return "VALUE";
	case MARK:
		return "MARK";
	case DATA:
		return "DATA";
	case ADDRESS:
		return "ADDRESS";
	case OTHER:
		return "OTHER";
	case NONE:
		return "NONE";
	}
}

std::shared_ptr<vector<assembler::token_t>> assembler::tokenize(shared_ptr<vector<string>> const& file)
{
	shared_ptr<vector<token_t>> res = make_shared<vector<token_t>>();

	for (size_t linePos = 0; linePos < file->size(); linePos++)
	{
		string currentLine = (*file)[linePos];
		// Converting to lowercase. Using the copy of string
		std::transform(currentLine.begin(), currentLine.end(), currentLine.begin(), ::tolower);

		if (currentLine.empty()) break; // E.g. new lines
		else if (currentLine[0] == '-' && currentLine[1] == '-' || // Comments
			currentLine[0] == '/' && currentLine[1] == '/' ||
			currentLine[0] == ';')
		{
			continue;
		}

		stringstream buf; // For separate words

		for(size_t textPos = 0; textPos <= currentLine.size(); textPos++)
		{
			if((currentLine[textPos] != ' ' && currentLine[textPos] != '\t' && 
				currentLine[textPos] != '\r' && currentLine[textPos] != ',') && (textPos != currentLine.size()))
			{
				buf << currentLine[textPos];
			}
			else 
			{
				string work = buf.str();

				if (work.empty()) continue;
				else if (work[0] == '-' && work[1] == '-' || // Comments which on the line with command
					work[0] == '/' && work[1] == '/' ||
					work[0] == ';')
				{
					break;
				}
				// Cheking for being a command
				if (keywords.find(work) != keywords.end())
				{
					res->push_back({ COMMAND, work, textPos - work.size(), linePos+1 });
				}
				else if (work[work.size() - 1] == ':') // Mark
				{
					res->push_back({ MARK, work.substr(0, work.size() - 1), textPos - work.size(), linePos + 1 });
				}
				else if (work[0] == 'v') // Register
				{
					res->push_back({ REGISTER, work, textPos - work.size(), linePos + 1 });
				}
				else if (work[0] == '$') // Get value from ...
				{
					res->push_back({ DATA, work.substr(1, work.size() - 1), textPos - work.size(), linePos + 1 });
				}
				else if (work[0] == '[') // Get address of mark or register I
				{
					res->push_back({ ADDRESS, work.substr(1, work.size() - 2), textPos - work.size(), linePos + 1 });
				}
				else if (isdigit(work[0])) // Number
				{
					res->push_back({ VALUE, work, textPos - work.size(), linePos + 1 });
				}
				else
				{
					res->push_back({ OTHER, work, textPos - work.size(), linePos + 1 });
				}

				// Necessarily
				buf.str("");
				buf.clear();
			}
		}
	}
	return res;
}

program parser::parse()
{
	program res = make_unique<vector<unique_ptr<expression>>>();
	while (parserPos < tokens->size())
	{
		token t = parserMatch(MARK);
		if (t.type != NONE)
		{
			size_t pos = 0x200;
			for (size_t i = 0; i < res->size(); i++)
			{
				pos += (*res)[i]->size();
			}

			res->push_back(move(parseCmd()));
			scope[t.str] = pair<dbyte, dbyte>(pos, 0);
			if ((*res)[res->size() - 1]->getCommand().str == "dw")
				scope[t.str].second = (*res)[res->size() - 1]->codegen(scope);
		}
		else
		{
			res->push_back(move(parseCmd()));
		}
		
	}
	return move(res);
}

static set<string> noArgsCmds = {"cls", "ret"};
static set<string> oneArgsCmds = {"call", "skp", "sknp", "shr", "shl", "dw"};
static set<string> twoArgsCmds = {"se", "sne", "ld", "add", "or", "xor", 
								  "sub", "subn", "rnd", "and"};

token assembler::parser::parserMatch(token_type expected)
{
	if (parserPos < tokens->size())
	{
		token currentToken = (*tokens)[parserPos];
		if (currentToken.type == expected)
		{
			parserPos++;
			parserCodePos = currentToken.pos;
			parserLinePos = currentToken.line;
			return currentToken;
		}
	}
	return token_t();
}

token assembler::parser::parserRequire(token_type expected) 
{
	token currentToken = parserMatch(expected);
	if (currentToken.type != expected)
		throw assembler_exception("expected " + token_type_to_string(expected), 
			parserLinePos+1, parserCodePos);
	return currentToken;
}

token assembler::parser::parserRequire()
{
	if (parserPos < tokens->size())
	{
		token currentToken = (*tokens)[parserPos];
		if (currentToken.type == COMMAND)
			throw assembler_exception("expected argument, not COMMAND" + to_string((*tokens)[parserPos+1].pos),
				parserLinePos+1, parserCodePos);
		parserPos++;
		parserCodePos = currentToken.pos;
		return currentToken;
	}
	else
		throw assembler_exception("expecting argument", parserLinePos+1, parserCodePos);
}

token assembler::parser::parserNext()
{
	if (parserPos < tokens->size())
	{
		token currentToken = (*tokens)[parserPos];
		parserPos++;
		parserCodePos = currentToken.pos;
		return currentToken;
	}
	else
		throw assembler_exception("parser reached end", 0, 0);
}

unique_ptr<expression> parser::parseCmd()
{
	token cmd = parserNext();

	if (noArgsCmds.find(cmd.str) != noArgsCmds.end())
		return move(make_unique<expression>(cmd));
	else if (oneArgsCmds.find(cmd.str) != oneArgsCmds.end())
		return move(make_unique<expression>(cmd, parserRequire()));
	else if (twoArgsCmds.find(cmd.str) != twoArgsCmds.end()) // Arguments evaluation order!
	{
		token first = parserRequire();
		token second = parserRequire();
		return move(make_unique<expression>(cmd, first, second));
	}
	else if (cmd.str == "jp") // Special case
	{
		token first = parserRequire();
		token second = parserMatch(VALUE);
		return move(make_unique<expression>(cmd, first, second));
	}
	else if (cmd.str == "drw")
	{
		token first = parserRequire(REGISTER);
		token second = parserRequire(REGISTER);
		token third = parserRequire(VALUE);
		return move(make_unique<expression>(cmd, first, second, third));
	}
	else throw assembler_exception("there is no such command as " + cmd.str, cmd.line, cmd.pos);
}

#define CODEGEN_THROW(token1, expected) throw assembler_exception("expected " \
										+ token_type_to_string(expected), token1.line, token1.pos)

#define CODEGEN_THROW2(token1, expected1, expected2) throw assembler_exception("expected " \
													+ token_type_to_string(expected1) + " or " \
													+ token_type_to_string(expected2), \
													token1.line, token1.pos)

#define CODEGEN_THROW3(token1, expected1, expected2, expected3) throw assembler_exception("expected " + \
													token_type_to_string(expected1) + " or " + \
													token_type_to_string(expected2) + " or " + token_type_to_string(expected3), \
													token1.line, token1.pos)

#define CODEGEN_EXPECT(token1, expected) (token1.type == expected)

#define CODEGEN_ASSERT(token1, expected) if(!CODEGEN_EXPECT(token1, expected)) CODEGEN_THROW(token1, expected)
#define CODEGEN_ASSERT2(token1, expected1, expected2) if(!CODEGEN_EXPECT(token1, expected1) || !CODEGEN_EXPECT(token1, expected2)) \
														CODEGEN_THROW2(token1, expected, expected2)
#define CODEGEN_ASSERT3(token1, expected1, expected2, expected3) if(!CODEGEN_EXPECT(token1, expected1) || !CODEGEN_EXPECT(token1, expected2) \
												|| !CODEGEN_EXPECT(token1, expected3)) CODEGEN_THROW3(token1, expected1, expected2, expected3)

#define CODEGEN_EXPECT_VALUE(token1) (token1.type == VALUE || token1.type == ADDRESS || token1.type == DATA)
#define CODEGEN_ASSERT_VALUE(token1) if(!CODEGEN_EXPECT_VALUE(token1)) CODEGEN_THROW3(token1, VALUE, DATA, ADDRESS)

int assembler::expression::codegen(context scope)
{
	if (cmd.str == "cls") return 0x00E0;
	else if (cmd.str == "ret") return 0x00EE;
	else if (cmd.str == "jp")
	{
		if (CODEGEN_EXPECT_VALUE(arg1)) return 0x1000 + parser::parseValue(arg1, scope, 0xFFF); // 0x1nnn
		else if (arg1.type == REGISTER)
		{
			if (parser::parseReg(arg1) != 0)
				throw assembler_exception("only v0 is supported", arg1.line, arg1.pos);
			CODEGEN_ASSERT_VALUE(arg2);
			return 0xB000 + parser::parseValue(arg2, scope, 0xFFF); // 0xBnnn
		}
		else CODEGEN_THROW(arg1, VALUE);
	}
	else if (cmd.str == "call")
	{
		CODEGEN_ASSERT_VALUE(arg1);
		return 0x2000 + parser::parseValue(arg1, scope, 0xFFF); // 0x2nnn
	}
	else if (cmd.str == "skp")
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		return 0xE000 + parser::parseReg(arg1) * 0x100 + 0x9E; // 0xEx9E
	}
	else if (cmd.str == "sknp")
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		return 0xE000 + parser::parseReg(arg1) * 0x100 + 0xA1; // 0xExA1
	}
	else if (cmd.str == "shl")
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		return 0x8000 + parser::parseReg(arg1) * 0x100 + 0x0E; // 0x8xyE
	}
	else if (cmd.str == "shr")
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		return 0x8000 + parser::parseReg(arg1) * 0x100 + 0x06; // 0x8xy6
	}
	else if (cmd.str == "dw")
	{
		CODEGEN_ASSERT_VALUE(arg1);
		return parser::parseValue(arg1, scope, 0xFFFF);
	}
	else if (cmd.str == "se") // 0x3xkk || 0x5xy0
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		if (CODEGEN_EXPECT_VALUE(arg2)) return 0x3000 + parser::parseReg(arg1) * 0x100 + parser::parseValue(arg2, scope, 0xFF);
		else if (CODEGEN_EXPECT(arg1, REGISTER)) return 0x5000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10;
		else CODEGEN_THROW2(arg2, VALUE, REGISTER);
	}
	else if (cmd.str == "sne") // 0x4xkk || 0x9xy0
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		if (CODEGEN_EXPECT_VALUE(arg2)) return 0x4000 + parser::parseReg(arg1) * 0x100 + parser::parseValue(arg2, scope, 0xFF);
		else if (arg2.type == REGISTER) return 0x9000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10;
		else CODEGEN_THROW2(arg2, VALUE, REGISTER);
	}
	else if (cmd.str == "add") // 0x7xkk || 0x8xy4 || 0xFx1E
	{
		if (arg1.type != REGISTER && arg1.str != "i") CODEGEN_THROW(arg1, REGISTER);
		if (CODEGEN_EXPECT_VALUE(arg2)) return 0x7000 + parser::parseReg(arg1) * 0x100 + parser::parseValue(arg2, scope, 0xFF);
		else if (arg2.type == REGISTER)
		{
			if(arg1.str == "i") return 0xF000 + parser::parseReg(arg2) * 0x100 + 0x1E;
			return 0x8000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10 + 0x4;
		}
		else CODEGEN_THROW2(arg2, VALUE, REGISTER);
	}
	else if (cmd.str == "or") // 0x8xy1
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		CODEGEN_ASSERT(arg2, REGISTER);
		return 0x8000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10 + 0x1;
	}
	else if (cmd.str == "and") // 0x8xy2
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		CODEGEN_ASSERT(arg2, REGISTER);
		return 0x8000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10 + 0x2;
	}
	else if (cmd.str == "xor") // 0x8xy3
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		CODEGEN_ASSERT(arg2, REGISTER);
		return 0x8000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10 + 0x3;
	}
	else if (cmd.str == "sub") // 0x8xy5
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		CODEGEN_ASSERT(arg2, REGISTER);
		return 0x8000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10 + 0x5;
	}
	else if (cmd.str == "subn") // 0x8xy7
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		CODEGEN_ASSERT(arg2, REGISTER);
		return 0x8000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10 + 0x7;
	}
	else if (cmd.str == "rnd") // 0xCxkk
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		CODEGEN_ASSERT_VALUE(arg2);
		return 0xC000 + parser::parseReg(arg1) * 0x100 + parser::parseValue(arg2, scope, 0xFF);
	}
	else if (cmd.str == "drw") // 0xDxyn
	{
		CODEGEN_ASSERT(arg1, REGISTER);
		CODEGEN_ASSERT(arg2, REGISTER);
		CODEGEN_ASSERT_VALUE(arg3);
		return 0xD000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10 + parser::parseValue(arg3, scope, 0xF);
	}
	else if (cmd.str == "ld")
	{
		if (CODEGEN_EXPECT(arg1, REGISTER) && CODEGEN_EXPECT(arg2, REGISTER)) // 0x8xy0
		{
			return 0x8000 + parser::parseReg(arg1) * 0x100 + parser::parseReg(arg2) * 0x10;
		}
		else if (CODEGEN_EXPECT(arg1, REGISTER) && CODEGEN_EXPECT_VALUE(arg2)) // 0x6xkk || 0xFx65
		{
			if(arg2.str == "i") return 0xF000 + parser::parseReg(arg1) * 0x100 + 0x65;
			else return 0x6000 + parser::parseReg(arg1) * 0x100 + parser::parseValue(arg2, scope, 0xFF);
		}
		else if (arg1.str == "i" && arg1.type == OTHER && CODEGEN_EXPECT_VALUE(arg2)) // 0xAnnn
		{
			return 0xA000 + parser::parseValue(arg2, scope, 0xFFF);
		}
		else if (CODEGEN_EXPECT(arg1, REGISTER) && arg2.str == "dt") // 0xFx07
		{
			return 0xF000 + parser::parseReg(arg1) * 0x100 + 0x07;
		}
		else if (CODEGEN_EXPECT(arg1, REGISTER) && arg2.str == "k") // 0xFx0A
		{
			return 0xF000 + parser::parseReg(arg1) * 0x100 + 0x0A;
		}
		else if (arg1.str == "dt" && CODEGEN_EXPECT(arg2, REGISTER)) // 0xFx15
		{
			return 0xF000 + parser::parseReg(arg2) * 0x100 + 0x15;
		}
		else if (arg1.str == "st" && CODEGEN_EXPECT(arg2, REGISTER)) // 0xFx18
		{
			return 0xF000 + parser::parseReg(arg2) * 0x100 + 0x18;
		}
		else if (arg1.str == "f" && CODEGEN_EXPECT(arg2, REGISTER)) // 0xFx29
		{
			return 0xF000 + parser::parseReg(arg2) * 0x100 + 0x29;
		}
		else if (arg1.str == "b" && CODEGEN_EXPECT(arg2, REGISTER)) // 0xFx33
		{
			return 0xF000 + parser::parseReg(arg2) * 0x100 + 0x33;
		}
		else if (arg1.str == "i" && arg1.type == ADDRESS && CODEGEN_EXPECT(arg2, REGISTER)) // 0xFx55
		{
			return 0xF000 + parser::parseReg(arg2) * 0x100 + 0x55;
		}
		else
		{
			throw assembler_exception("[" + to_string(cmd.line) + "]: can't find proper ld command form on pos " + to_string(cmd.pos), cmd.line, cmd.pos);
		}
	}

	return 0xFFFF;
}

int parser::parseValue(token t, context scope, int max)
{
	int res;
	if (scope.count(t.str))
	{
		if (t.type == ADDRESS)
		{
			res = scope.at(t.str).first;
		}
		else
		{
			res = scope.at(t.str).second;
		}
	}
	else if(t.type != ADDRESS && t.type != DATA)
	{
		res = stoi(t.str, nullptr, 0);
	}
	else
	{
		throw assembler_exception("there is no such mark as \"" + t.str + "\"", t.line, t.pos);
	}
	
	if (res > max || res < 0) throw assembler_exception("too big value", t.line, t.pos);
	return res;
}

int parser::parseReg(token t)
{
	try
	{
		return stoi(string(1, t.str[1]), nullptr, 16);
	}
	catch (...)
	{
		throw assembler_exception("can't parse register", t.line, t.pos);
	}
}