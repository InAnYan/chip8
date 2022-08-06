#ifndef NEW_C8ASM_HPP
#define NEW_C8ASM_HPP

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <cstdint>
#include <sstream>
#include <utility>

namespace c8asm
{
	enum class token_type
	{
		LABEL,
		REGISTER,
		VALUE, // literals
		ADDRESS, // addres of label '[]'
		DATA, // value of label '$'
		COMMAND,

		NONE
	};

	typedef struct
	{
		token_type type;
		std::string str;
		size_t line;
		size_t pos;
	} token_t;

	typedef const token_t& token;
	
	typedef std::shared_ptr<std::vector<token_t>> tokens;
	typedef std::shared_ptr<std::vector<uint16_t>> opcodes_program;
	
	tokens tokenize(const std::vector<std::string>& file);

	class generator
	{
	private:
		const tokens tokenList;
		size_t parserPos = 0;
		size_t nextAddress = 0;

		token_t NONE_TOKEN = { token_type::NONE, "", 0, 0 };

		std::map<std::string, size_t> labelsLookUp;
		std::map<std::string, uint16_t> constsLookUp;

		enum class job_type
		{
			NONE,
			PUT_ADDRESS,
			PUT_DATA
		};
		
		typedef struct
		{	
			uint16_t bytes;
			
			std::string label;
			unsigned max; // max value of data determined by instruction (for CHIP-8 it can by 0x00FF or 0x0FFF
			job_type type;
			size_t line;
		} bytes_job_t;
		std::vector<bytes_job_t> jobs;
		bytes_job_t currentJob;

		token match(token_type type);
		token require(token_type type);

		// gets raw value or creates job to put it
		uint16_t getValue(unsigned maxValue);
		// gets raw address or creates job to put it
		uint16_t getAddress();
		// requires and parses register
		uint8_t getReg();
		// parses register
		uint8_t parseReg(token reg);

		void generateJobs(); // First pass
		opcodes_program processJobs(); // Second pass

		std::string generateExpectedMsg(token_type type, size_t line, size_t pos);
		
	public:
		generator(tokens program) : tokenList(program) {}

		opcodes_program generateBytes();
	};


	class generator_exception : public std::runtime_error
	{
	private:
		std::pair<size_t, size_t> position;
	public:
		generator_exception(const std::string& msg, size_t line, size_t pos) : runtime_error(msg), position(line, pos) {}
		
		const std::pair<size_t, size_t>& where() const { return position; }
	};
}

#endif
