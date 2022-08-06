#include "newc8asm.hpp"

#include <set>
#include <algorithm>



#include <iomanip>
#include <iostream>




namespace c8asm
{
	std::set<std::string> chip8Instructions = {"cls", "ret", "ld", "and", "or", "xor",
			"call", "se", "sne", "add", "sub", "shr", "shl", "subn", "dw",
			"jp", "rnd", "drw", "skp", "sknp", "const"};
	
	tokens tokenize(const std::vector<std::string>& file)
	{	
		std::stringstream buff;
		tokens res = std::make_shared<std::vector<token_t>>(); 
		
		for(size_t lineNum = 0; lineNum < file.size(); lineNum++)
		{
			std::string currentLine = file[lineNum];
			std::transform(currentLine.begin(), currentLine.end(), currentLine.begin(), ::tolower);

			if (currentLine.empty()) continue; // E.g. new lines
			else if (currentLine[0] == '-' && currentLine[1] == '-' || // Comments
					 currentLine[0] == '/' && currentLine[1] == '/' ||
					 currentLine[0] == ';')
			{
				continue;
			}
			
			for(size_t linePos = 0; linePos <= currentLine.size(); linePos++)
			{
#define CURRENT_CHARACTER currentLine[linePos]
#define WHILE_NOT_SPACE_CHARACTERS (CURRENT_CHARACTER != ' ' && CURRENT_CHARACTER != ',' && CURRENT_CHARACTER != '\t' && CURRENT_CHARACTER != '\r')
				
				if(WHILE_NOT_SPACE_CHARACTERS && (linePos != currentLine.size()))
				{
					buff << CURRENT_CHARACTER;
				}
				else
				{
					std::string currentStr = buff.str();
					
				    if (currentStr.empty()) continue;
					else if (currentStr[0] == '-' && currentStr[1] == '-' ||
							 currentStr[0] == '/' && currentStr[1] == '/' ||
							 currentStr[0] == ';')
					{
						buff.str("");
						buff.clear();
						break;
					}
					size_t currentPos = (linePos+1) - currentStr.size();
					
					buff.str("");
					buff.clear();
					
					token_t currentToken;
					currentToken.type = token_type::REGISTER;
					currentToken.str = currentStr;
					currentToken.line = lineNum;
					currentToken.pos = currentPos;
					if (chip8Instructions.find(currentStr) != chip8Instructions.end())
					{
						currentToken.type = token_type::COMMAND;
					}
					else if (isdigit(currentStr[0]))
					{
						currentToken.type = token_type::VALUE;
					}
					else if (currentStr[0] == '$')
					{
					    currentToken.type = token_type::DATA;
						currentToken.str = currentStr.substr(1, currentStr.size()-1);
					}
					else if (currentStr[0] == '[')
					{
						currentToken.type = token_type::ADDRESS;
						currentToken.str = currentStr.substr(1, currentStr.size()-2);
					}
					else if (currentStr[currentStr.size()-1] == ':')
					{
						currentToken.type = token_type::LABEL;
						currentToken.str = currentStr.substr(0, currentStr.size()-1);
					}

					res->push_back(currentToken);
				}
				
#undef CURRENT_CHARACTER
#undef WHILE_NOT_SPACE_CHARACTER
				
			}
		}
		
		return res;
	}

	opcodes_program generator::generateBytes()
	{	
		generateJobs();
		return processJobs();
	}

	token generator::match(token_type type)
	{
		if(parserPos < tokenList->size())
		{
			token res = (*tokenList)[parserPos];
			if(res.type == type)
			{
				parserPos++;
				return res;
			}
			return NONE_TOKEN;
		}
		else
		{
			throw std::out_of_range("parser reached end");
		}
	}

	token generator::require(token_type type)
	{
		token res = match(type);
		if(res.type == token_type::NONE)
			throw generator_exception(generateExpectedMsg(type, (*tokenList)[parserPos].line, (*tokenList)[parserPos].pos),
									  (*tokenList)[parserPos].line, (*tokenList)[parserPos].pos);
		return res;
	}
		
	void generator::generateJobs()
	{
		while(parserPos < tokenList->size())
		{
			// Mark
			token label = match(token_type::LABEL);
			if (label.type != token_type::NONE)
			{
			    if (labelsLookUp.count(label.str) == 0)
				{
					labelsLookUp[label.str] = nextAddress;
				}
				else
				{
					throw generator_exception("label redefinition: \"" + label.str + "\"", label.line, label.pos);
				}
			}
				
			// and/or command
			token cmd = require(token_type::COMMAND);
			currentJob.bytes = 0;
			currentJob.label = "";
			currentJob.type = job_type::NONE;
			currentJob.max = 0;
			currentJob.line = cmd.line;
			uint16_t& bytes = currentJob.bytes;
				
			if (cmd.str == "cls")
			{
				bytes = 0x00E0;
			}
			else if (cmd.str == "ret")
			{
				bytes = 0x00EE;
			}
			else if (cmd.str == "jp")
			{
				token maybeReg = match(token_type::REGISTER);
				if(maybeReg.type != token_type::NONE)
				{
					uint16_t reg = parseReg(maybeReg);
					if (reg != 0) throw generator_exception("only v0 supported", maybeReg.line, maybeReg.pos);
					bytes = 0xB000 + getAddress();
				}
				else
				{
					bytes = 0x1000 + getAddress();
				}
			}
			else if (cmd.str == "call")
			{
				bytes = 0x2000 + getAddress();
			}
			else if (cmd.str == "se")
			{
				uint8_t reg1 = getReg();
				token reg2 = match(token_type::REGISTER);
				if (reg2.type != token_type::NONE)
				{
					bytes = 0x5000 + reg1*0x100 + parseReg(reg2)*0x10;
				}
				else
				{
					bytes = 0x3000 + reg1*0x100 + getValue(0xFF);
				}
			}
			else if (cmd.str == "sne")
			{
				uint8_t reg1 = getReg();
				token reg2 = match(token_type::REGISTER);
				if (reg2.type != token_type::NONE)
				{
					bytes = 0x9000 + reg1*0x100 + parseReg(reg2)*0x10;
				}
				else
				{
					bytes = 0x4000 + reg1*0x100 + getValue(0xFF);
				}
			}
			else if (cmd.str == "add")
			{
				token reg1 = require(token_type::REGISTER);
				if (reg1.str == "i")
				{
					bytes = 0xF01E + getReg()*0x100;
				}
				else
				{
					token reg2 = match(token_type::REGISTER);
					if (reg2.type != token_type::NONE)
					{
						bytes = 0x8000 + parseReg(reg1)*0x100 + parseReg(reg2)*0x10 + 0x4;
					}
					else
					{
						bytes = 0x7000 + parseReg(reg1)*0x100 + getValue(0xFF);
					}
				}
			}
			else if (cmd.str == "or")
			{
				bytes = 0x8000 + getReg()*0x100 + getReg()*0x10 + 0x1;
			}
			else if (cmd.str == "and")
			{
				bytes = 0x8000 + getReg()*0x100 + getReg()*0x10 + 0x2;
			}
			else if (cmd.str == "xor")
			{
				bytes = 0x8000 + getReg()*0x100 + getReg()*0x10 + 0x3;
			}
			else if (cmd.str == "sub")
			{
				bytes = 0x8000 + getReg()*0x100 + getReg()*0x10 + 0x5;
			}
			else if (cmd.str == "subn")
			{
				bytes = 0x8000 + getReg()*0x100 + getReg()*0x10 + 0x7;
			}
			else if (cmd.str == "shr")
			{
				bytes = 0x8000 + getReg()*0x100 + 0x6;
			}
			else if (cmd.str == "shl")
			{
				bytes = 0x8000 + getReg()*0x100 + 0xE;
			}
			else if (cmd.str == "rnd")
			{
				bytes = 0xC000 + getReg()*0x100 + getValue(0xFF);
			}
			else if (cmd.str == "skp")
			{
				bytes = 0xE09E + getReg()*0x100;
			}
			else if (cmd.str == "sknp")
			{
				bytes = 0xE0A1 + getReg()*0x100;
			}
			else if (cmd.str == "ld")
			{
				token reg1 = match(token_type::REGISTER);
				if (reg1.type != token_type::NONE)
				{
					if (reg1.str == "dt")
					{
						bytes = 0xF015 + getReg()*0x100;
					}
					else if (reg1.str == "st")
					{
						bytes = 0xF018 + getReg()*0x100;
					}
					else if (reg1.str == "i")
					{
						bytes = 0xA000 + getAddress();
					}
					else if (reg1.str == "f")
					{
						bytes = 0xF029 + getReg()*0x100;
					}
					else if (reg1.str == "b")
					{
						bytes = 0xF033 + getReg()*0x100;
					}
					else
					{
						token reg2 = match(token_type::REGISTER);
						if (reg2.type != token_type::NONE)
						{
							if (reg2.str == "dt")
							{
								bytes = 0xF007 + parseReg(reg1)*0x100;
							}
							else if (reg2.str == "k")
							{
								bytes = 0xF00A + parseReg(reg1)*0x100;
							}
							else
							{
								bytes = 0x8000 + parseReg(reg1)*0x100 + parseReg(reg2)*0x10;
							}
						}
						else
						{
							token addr = match(token_type::ADDRESS);
							if (addr.type != token_type::NONE)
							{
								if (addr.str != "i")
									throw generator_exception("only instrucion register supported", addr.line, addr.pos);
								bytes = 0xF065 + parseReg(reg1)*0x100;
							}
							else
							{
								bytes = 0x6000 + parseReg(reg1)*0x100 + getValue(0xFF);
							}
						}
					}
				}
				else
				{
					token addr = require(token_type::ADDRESS);
					if (addr.str != "i") throw generator_exception("only instrucion register supported", addr.line, addr.pos);
					bytes = 0xF055 + getReg()*0x100;
				}
			}
			else if (cmd.str == "drw")
			{
				bytes = 0xD000 + getReg()*0x100 + getReg()*0x10 + getValue(0xF);
			}
			else if (cmd.str == "const")
			{
				token constant = require(token_type::REGISTER);
				token value = require(token_type::VALUE);
				if (constsLookUp.count(constant.str) == 0)
				{
					uint16_t res;
					try
					{
						res = stoi(value.str, nullptr, 0);
					}
					catch (...)
					{
						throw generator_exception("can't parse constant", constant.line, constant.pos);
					}
					constsLookUp[constant.str] = res;
				}
				else
				{
					throw generator_exception("constant redefinition: \"" +
											  constant.str + "\"", constant.line, constant.pos);
				}
			}
			else if (cmd.str == "dw")
			{
				bytes = getValue(0xFFFF);
			}
			else
			{
				throw generator_exception("unknown command", cmd.line, cmd.pos);
			}
				
			jobs.push_back(currentJob);
			nextAddress += 2;
		}
	}
	
	uint16_t generator::getValue(unsigned maxValue)
	{
		token data = match(token_type::DATA);
		if (data.type != token_type::NONE)
		{
			currentJob.type = job_type::PUT_DATA;
			currentJob.max = maxValue;
			currentJob.label = data.str;
			return 0;
		}
		else
		{
			token value = require(token_type::VALUE);

			uint16_t res;
			try
			{
				res = stoi(value.str, nullptr, 0);
			}
			catch (...)
			{
				throw generator_exception("can't parse value", value.line, value.pos);
			}
			
			if (res > maxValue)
				throw generator_exception("too big value", value.line, value.pos);
			
			return res;
		}
	}

	uint16_t generator::getAddress()
	{
		token addr = match(token_type::ADDRESS);
		if (addr.type != token_type::NONE)
		{
			currentJob.type = job_type::PUT_ADDRESS;
			currentJob.max = 0xFFF;
			currentJob.label = addr.str;

			return 0x200;
		}
		else
		{
			token value = require(token_type::VALUE);

			uint16_t res;
			try
			{
				res = stoi(value.str, nullptr, 0);
			}
			catch (...)
			{
				throw generator_exception("can't parse value", value.line, value.pos);
			}

			if (res > 0xFFF)
				throw generator_exception("too big value", value.line, value.pos);

			return res;
		}
	}
	
	uint8_t generator::getReg()
	{
		token reg = require(token_type::REGISTER);
		uint8_t res;
		try
		{
			res = stoi(reg.str.substr(1, reg.str.size()-1), nullptr, 16);
		}
		catch (...)
		{
			throw generator_exception("can't parse register", reg.line, reg.pos);
		}

		if (res > 0xF)
			throw generator_exception("there is no such register", reg.line, reg.pos);

		return res;
	}

	uint8_t generator::parseReg(token reg)
	{
		uint8_t res;
		try
		{
			res = stoi(reg.str.substr(1, reg.str.size()-1), nullptr, 16);
		}
		catch (...)
		{
			throw generator_exception("can't parse register", reg.line, reg.pos);
		}

		if (res > 0xF)
			throw generator_exception("there is no such register", reg.line, reg.pos);

		return res;
	}
	
	std::string generator::generateExpectedMsg(token_type type, size_t line, size_t pos)
	{
		std::stringstream buff;
		buff << "expected ";

		switch(type)
		{
		case token_type::REGISTER:
			buff << "REGISTER";
			break;
		case token_type::VALUE:
			buff << "VALUE";
			break;
		case token_type::ADDRESS:
			buff << "ADDRESS";
			break;
		case token_type::COMMAND:
			buff << "COMMAND";
			break;
		default:
			buff << "(can generator expect this? : " << (int)type << ")";
			break;
		}

		buff << " on line " << line << " on pos " << pos;

		return buff.str();
	}

	opcodes_program generator::processJobs()
	{		
		opcodes_program res = std::make_shared<std::vector<uint16_t>>();
		res->reserve(jobs.size());
		for (auto const& job : jobs)
		{
			if (job.type == job_type::PUT_ADDRESS)
			{
				uint16_t addr;
				if (labelsLookUp.count(job.label))
				{
					addr = labelsLookUp[job.label];
					if (addr > job.max)
						throw generator_exception("address too big, probably program too large", job.line, 0);
					res->push_back(job.bytes + addr);
				}
				else
				{
					throw generator_exception("there is no such label as: \"" + job.label + "\"", job.line, 0);
				}
			}
			else if (job.type == job_type::NONE)
			{
				res->push_back(job.bytes);
			}
			else if (job.type == job_type::PUT_DATA)
			{
				uint16_t data;
				if (constsLookUp.count(job.label))
				{
					data = constsLookUp[job.label];
					if (data > job.max)
						throw generator_exception("constant too big, probably program too large", job.line, 0);
					res->push_back(job.bytes + data);
				}
				else
				{
					throw generator_exception("there is no such constant as: \"" + job.label + "\"", job.line, 0);
				}
			}
		}
		
		return res;
	}
}

