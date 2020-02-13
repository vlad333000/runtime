#include "logging.h"
#include "parsing/parsepreprocessor.h"
#include "parsing/position_info.h"
#include "parsing/astnode.h"
#include "instruction.h"
#include <iostream>
#include <sstream>
using namespace std::string_view_literals;

#pragma region StreamLogger
StreamLogger::StreamLogger(std::ostream& target): logTarget(target) {
    std::fill(enabledWarningLevels.begin(), enabledWarningLevels.end(), true);
}
void StreamLogger::log(loglevel level, std::string_view message) {
    std::unique_lock lock(streamLock);
    logTarget << Logger::loglevelstring(level) << ' ' << message << std::endl;
}
#pragma endregion StreamLogger
#pragma region StdOutLogger
void StdOutLogger::log(loglevel level, std::string_view message) {
	auto& logTarget = std::cout;
    logTarget << Logger::loglevelstring(level) << ' ' << message << std::endl;
}
#pragma endregion StdOutLogger

#pragma region LogLocationInfo

LogLocationInfo::LogLocationInfo(const sqf::parse::preprocessorfileinfo& info) {
    path = info.path;
    line = info.line;
    col = info.col;
}
LogLocationInfo::LogLocationInfo(const sqf::parse::astnode& node) {
    path = node.file;
    line = node.line;
    col = node.col;
}

LogLocationInfo::LogLocationInfo(const sqf::parse::position_info& info)
{
	path = info.file;
	line = info.line;
	col = info.column;
}

LogLocationInfo::LogLocationInfo(const sqf::instruction& info) {
    path = info.file();
    line = info.line();
    col = info.col();
}

std::string LogLocationInfo::format() const {
    std::string output;

    output.reserve(
        path.length()
        + 6 //col/line good average
        + "[L"sv.length()
        + "|C"sv.length()
        + (path.empty() ? 0 : "|"sv.length())
        + "]"sv.length()
        + "\t"sv.length()
    );

    output.append("[L"sv);
    output.append(std::to_string(line));
    output.append("[C"sv);
    output.append(std::to_string(col));
    if (!path.empty()) {
        output.append("|"sv);
        output.append(path);
    }

    output.append("]\t"sv);

    return output;
}

#pragma endregion LogLocationInfo

void CanLog::log(LogMessageBase& message) const {
    if (!m_logger.isEnabled(message.getLevel())) return;
	m_logger.log(message.getLevel(), message.formatMessage());


    //log(logmessage::preprocessor::ArgCountMissmatch(LogLocationInfo()));

}
void CanLog::log(LogMessageBase&& message) const {
    if (!m_logger.isEnabled(message.getLevel())) return;
	m_logger.log(message.getLevel(), message.formatMessage());


    //log(logmessage::preprocessor::ArgCountMissmatch(LogLocationInfo()));

}


namespace logmessage::preprocessor {

    std::string ArgCountMissmatch::formatMessage() const {
        auto output = location.format();
        auto const message = "Arg Count Missmatch."sv;

        output.reserve(output.length() + message.length());
        output.append(message);
        return output;
    }

    std::string UnexpectedDataAfterInclude::formatMessage() const {
        auto output = location.format();
        auto const message = "Unexpected data after include path."sv;

        output.reserve(output.length() + message.length());
        output.append(message);
        return output;

    }

    std::string RecursiveInclude::formatMessage() const {
        auto output = location.format();
        auto const message = "Recursive include detected. Include Tree:"sv;

        output.reserve(output.length() + message.length() + includeTree.length());
        output.append(message);
        output.append(includeTree);
        return output;
    }

    std::string IncludeFailed::formatMessage() const {
        auto output = location.format();

        const std::string_view exceptionMessage(exception.what());

        output.reserve(
            output.length() 
            + "Failed to include '"sv.length()
            + line.length()
            + "' into file '"sv.length()
            + location.path.length()
            + "']\t':"sv.length()
            + exceptionMessage.length()
        );
        output.append("Failed to include '"sv);
        output.append(line);
        output.append("' into file '"sv);
        output.append(location.path);
        output.append("']\t':"sv);
        output.append(exceptionMessage);
        return output;

    }

    std::string MacroDefinedTwice::formatMessage() const {
        auto output = location.format();

        output.reserve(
            output.length() 
            + "Macro '"sv.length()
            + macroname.length()
            + "' defined twice."sv.length()
        );

        output.append("Macro '"sv);
        output.append(macroname);
        output.append("' defined twice."sv);
        return output;
    }

    std::string MacroNotFound::formatMessage() const {
        auto output = location.format();

        output.reserve(
            output.length() 
            + "Macro '"sv.length()
            + macroname.length()
            + "' not found."sv.length()
        );

        output.append("Macro '"sv);
        output.append(macroname);
        output.append("' not found."sv);
        return output;
    }

    std::string UnexpectedIfdef::formatMessage() const {
        auto output = location.format();
        const auto message = "Unexpected IFDEF. Already inside of a IFDEF or IFNDEF enclosure."sv;

        output.reserve(
            output.length() 
            + message.length()
        );

        output.append(message);
        return output;
    }

    std::string UnexpectedIfndef::formatMessage() const {
        auto output = location.format();
        const auto message = "Unexpected IFNDEF. Already inside of a IFDEF or IFNDEF enclosure."sv;

        output.reserve(
            output.length() 
            + message.length()
        );

        output.append(message);
        return output;
    }

    std::string UnexpectedElse::formatMessage() const {
        auto output = location.format();
        const auto message = "Unexpected ELSE. Not inside of a IFDEF or IFNDEF enclosure."sv;

        output.reserve(
            output.length() 
            + message.length()
        );

        output.append(message);
        return output;
    }

    std::string UnexpectedEndif::formatMessage() const {
        auto output = location.format();
        const auto message = "Unexpected ENDIF. Not inside of a IFDEF or IFNDEF enclosure."sv;

        output.reserve(
            output.length() 
            + message.length()
        );

        output.append(message);
        return output;
    }
    std::string MissingEndif::formatMessage() const {
        auto output = location.format();
        const auto message = "Missing ENDIF. Still inside of a IFDEF or IFNDEF enclosure at end of file."sv;

        output.reserve(
            output.length() 
            + message.length()
        );

        output.append(message);
        return output;
    }

    std::string UnknownInstruction::formatMessage() const {
        auto output = location.format();

        output.reserve(
            output.length() 
            + "Unknown PreProcessor instruction '"sv.length()
            + instruction.length()
            + "'."sv.length()
        );

        output.append("Unknown PreProcessor instruction '"sv);
        output.append(instruction);
        output.append("'."sv);
        return output;
    }

}

namespace logmessage::assembly {
	std::string ExpectedSemicolon::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected Semicolon."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NoViableAlternativeInstructions::formatMessage() const
	{
		auto output = location.format();
		const auto message = "No viable alternative at Instruction Path."sv 
			"Expected: ENDSTATEMENT or CALLUNARY or CALLBINARY or ASSIGNTO" 
			"or ASSIGNTOLOCAL or CALLNULAR or GETVARIABLE or MAKEARRAY or PUSH"sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NoViableAlternativeArg::formatMessage() const
	{
		auto output = location.format();
		const auto message = "No viable alternative at Instruction Path."sv
			"Expected: CALLUNARY or CALLBINARY or ASSIGNTO"
			"or ASSIGNTOLOCAL or CALLNULAR or GETVARIABLE or MAKEARRAY"sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedEndStatement::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'endStatement'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedCallNular::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'callNular'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedNularOperator::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected name of nular operator."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string UnknownNularOperator::formatMessage() const
	{
		auto output = location.format();

		output.reserve(
			output.length()
			+ "Unknown nular operator '"sv.length()
			+ operator_name.length()
			+ "'."sv.length()
		);

		output.append("Unknown nular operator '"sv);
		output.append(operator_name);
		output.append("'."sv);
		return output;
	}
	std::string ExpectedCallUnary::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'callUnary'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedUnaryOperator::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected name of unary operator."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string UnknownUnaryOperator::formatMessage() const
	{
		auto output = location.format();

		output.reserve(
			output.length()
			+ "Unknown unary operator '"sv.length()
			+ operator_name.length()
			+ "'."sv.length()
		);

		output.append("Unknown unary operator '"sv);
		output.append(operator_name);
		output.append("'."sv);
		return output;
	}
	std::string ExpectedCallBinary::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'callBinary'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedBinaryOperator::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected name of binary operator."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string UnknownBinaryOperator::formatMessage() const
	{
		auto output = location.format();

		output.reserve(
			output.length()
			+ "Unknown binary operator '"sv.length()
			+ operator_name.length()
			+ "'."sv.length()
		);

		output.append("Unknown binary operator '"sv);
		output.append(operator_name);
		output.append("'."sv);
		return output;
	}
	std::string ExpectedAssignTo::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'assignTo'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedVariableName::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected variable name."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedAssignToLocal::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'assignToLocal'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedGetVariable::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'getVariable'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedMakeArray::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'makeArray'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedInteger::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected integer."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedPush::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected literal 'push'."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedTypeName::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected type name."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NumberOutOfRange::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Number out of range. Creating NaN value."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
}

namespace logmessage::sqf
{
	std::string ExpectedStatementTerminator::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected Statement termination using `;` or `,`."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NoViableAlternativeStatement::formatMessage() const
	{
		auto output = location.format();
		const auto message = "No viable alternative for Statement. Expected Assignment or Expression."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingUnderscoreOnPrivateVariable::formatMessage() const
	{
		auto output = location.format();

		output.reserve(
			output.length()
			+ "Missing underscore (`_`) for private variable with '"sv.length()
			+ m_variable_name.length()
			+ "'."sv.length()
		);

		output.append("Missing underscore (`_`) for private variable with '"sv);
		output.append(m_variable_name);
		output.append("'."sv);
		return output;
	}
	std::string ExpectedBinaryExpression::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected Expression."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingRightArgument::formatMessage() const
	{
		// m_operator_name
		auto output = location.format();

		output.reserve(
			output.length()
			+ "Missing right argument on operator '"sv.length()
			+ m_operator_name.length()
			+ "'."sv.length()
		);

		output.append("Missing right argument on operator '"sv);
		output.append(m_operator_name);
		output.append("'."sv);
		return output;
	}
	std::string MissingRoundClosingBracket::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing round closing bracket (`)`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingCurlyClosingBracket::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing curly closing bracket (`}`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingSquareClosingBracket::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing square closing bracket (`]`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NoViableAlternativePrimaryExpression::formatMessage() const
	{
		auto output = location.format();
		const auto message = "No viable alternative for Primary Expression. Expected NUMBER or UNARYEXPRESSION or NULAREXPRESSION or VARIABLE or STRING or CODE or BRACKETS or ARRAY."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string EmptyNumber::formatMessage() const
	{
		auto output = location.format();
		const auto message = "The materialized numeric value is empty."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedSQF::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected SQF."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string EndOfFile::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Unexpected end-of-file reached."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
}

namespace logmessage::config
{
	std::string ExpectedStatementTerminator::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected Statement termination using `;`."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NoViableAlternativeNode::formatMessage() const
	{
		auto output = location.format();
		const auto message = "No viable alternative for Statement. Expected Confignode or Valuenode."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedIdentifier::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected Identifier."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingRoundClosingBracket::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing round closing bracket (`)`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingCurlyOpeningBracket::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing curly opening bracket (`{`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingCurlyClosingBracket::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing curly closing bracket (`}`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingSquareClosingBracket::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing square closing bracket (`]`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string MissingEqualSign::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Missing equal sign (`=`)."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedArray::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected Array."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExpectedValue::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Expected Value."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NoViableAlternativeValue::formatMessage() const
	{
		auto output = location.format();
		const auto message = "No viable alternative for Value. Expected String or Number or Localization or Array."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string EndOfFileNotReached::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Parsing terminated before end of file was reached. "sv
			"This usually means that your provided config contains errors that the parser could not catch."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
}
namespace logmessage::linting
{
	std::string UnassignedVariable::formatMessage() const
	{
		auto output = location.format();

		output.reserve(
			output.length()
			+ "Unassigned variable '"sv.length()
			+ m_variable_name.length()
			+ "'."sv.length()
		);

		output.append("Unassigned variable '"sv);
		output.append(m_variable_name);
		output.append("'."sv);
		return output;
	}
}
namespace logmessage::runtime
{
	std::string to_string(size_t num)
	{
		std::stringstream sstream;
		sstream << num;
		return sstream.str();
	}
	std::string to_cardinal_string(size_t num)
	{
		switch (num)
		{
		case 0:
			return "zero";
		case 1:
			return "one";
		case 2:
			return "two";
		case 3:
			return "three";
		case 4:
			return "four";
		case 5:
			return "five";
		case 6:
			return "six";
		case 7:
			return "seven";
		case 8:
			return "eight";
		case 9:
			return "nine";
		case 10:
			return "ten";
		case 11:
			return "eleven";
		case 12:
			return "twelve";
		default:
			std::stringstream sstream;
			sstream << num;
			return sstream.str();
			break;
		}
	}
	std::string to_ordinal_string(size_t num)
	{
		switch (num)
		{
		case 0:
			return "none";
		case 1:
			return "first";
		case 2:
			return "second";
		case 3:
			return "third";
		case 4:
			return "fourth";
		case 5:
			return "fifth";
		case 6:
			return "sixth";
		case 7:
			return "seventh";
		case 8:
			return "eighth";
		case 9:
			return "ninth";
		case 10:
			return "tenth";
		case 11:
			return "eleventh";
		case 12:
			return "twelfth";
		default:
			std::stringstream sstream;
			sstream << num << '.';
			return sstream.str();
			break;
		}
	}
	std::string Stacktrace::formatMessage() const
	{
		auto output = location.format();
		output.reserve(m_stacktrace.length());
		output.append(m_stacktrace);
		return output;
	}
	std::string MaximumInstructionCountReached::formatMessage() const
	{
		auto output = location.format();
		auto maximum_instruction_count = to_cardinal_string(m_maximum_instruction_count);

		output.reserve(
			output.length()
			+ "Maxium instruction count of "sv.length()
			+ maximum_instruction_count.length()
			+ " reached."sv.length()
		);

		output.append("Maxium instruction count of "sv);
		output.append(maximum_instruction_count);
		output.append(" reached."sv);
		return output;
	}
	std::string ExpectedArraySizeMissmatch::formatMessage() const
	{
		if (m_expected_min == m_expected_max)
		{
			auto output = location.format();
			auto expected_min = to_cardinal_string(m_expected_min);
			auto got = to_cardinal_string(m_got);

			output.reserve(
				output.length()
				+ "Array was expected to have "sv.length()
				+ expected_min.length()
				+ " elements but has "sv.length()
				+ got.length()
				+ "."sv.length()
			);

			output.append("Array was expected to have "sv);
			output.append(expected_min);
			output.append(" elements but has "sv);
			output.append(got);
			output.append("."sv);
			return output;
		}
		else
		{
			auto output = location.format();
			auto expected_min = to_cardinal_string(m_expected_min);
			auto expected_max = to_cardinal_string(m_expected_max);
			auto got = to_cardinal_string(m_got);

			output.reserve(
				output.length()
				+ "Array was expected to have "sv.length()
				+ expected_min.length()
				+ " to "sv.length()
				+ expected_max.length()
				+ " elements but has "sv.length()
				+ got.length()
				+ "."sv.length()
			);

			output.append("Array was expected to have "sv);
			output.append(expected_min);
			output.append(" to "sv);
			output.append(expected_max);
			output.append(" elements but has "sv);
			output.append(got);
			output.append("."sv);
			return output;
		}
	}
	std::string ExpectedArraySizeMissmatchWeak::formatMessage() const
	{
		if (m_expected_min == m_expected_max)
		{
			auto output = location.format();
			auto expected_min = to_cardinal_string(m_expected_min);
			auto got = to_cardinal_string(m_got);

			output.reserve(
				output.length()
				+ "Array was expected to have "sv.length()
				+ expected_min.length()
				+ " elements but has "sv.length()
				+ got.length()
				+ "."sv.length()
			);

			output.append("Array was expected to have "sv);
			output.append(expected_min);
			output.append(" elements but has "sv);
			output.append(got);
			output.append("."sv);
			return output;
		}
		else
		{
			auto output = location.format();
			auto expected_min = to_cardinal_string(m_expected_min);
			auto expected_max = to_cardinal_string(m_expected_max);
			auto got = to_cardinal_string(m_got);

			output.reserve(
				output.length()
				+ "Array was expected to have "sv.length()
				+ expected_min.length()
				+ " to "sv.length()
				+ expected_max.length()
				+ " elements but has "sv.length()
				+ got.length()
				+ "."sv.length()
			);

			output.append("Array was expected to have "sv);
			output.append(expected_min);
			output.append(" to "sv);
			output.append(expected_max);
			output.append(" elements but has "sv);
			output.append(got);
			output.append("."sv);
			return output;
		}
	}
	std::string ExpectedArraySizeMissmatchWeak::formatMessage() const
	{
		if (m_expected_min == m_expected_max)
		{
			auto output = location.format();
			auto expected_min = to_cardinal_string(m_expected_min);
			auto got = to_cardinal_string(m_got);

			output.reserve(
				output.length()
				+ "Array was expected to have "sv.length()
				+ expected_min.length()
				+ " elements but has "sv.length()
				+ got.length()
				+ "."sv.length()
			);

			output.append("Array was expected to have "sv);
			output.append(expected_min);
			output.append(" elements but has "sv);
			output.append(got);
			output.append("."sv);
			return output;
		}
		else
		{
			auto output = location.format();
			auto expected_min = to_cardinal_string(m_expected_min);
			auto expected_max = to_cardinal_string(m_expected_max);
			auto got = to_cardinal_string(m_got);

			output.reserve(
				output.length()
				+ "Array was expected to have "sv.length()
				+ expected_min.length()
				+ " to "sv.length()
				+ expected_max.length()
				+ " elements but has "sv.length()
				+ got.length()
				+ "."sv.length()
			);

			output.append("Array was expected to have "sv);
			output.append(expected_min);
			output.append(" to "sv);
			output.append(expected_max);
			output.append(" elements but has "sv);
			output.append(got);
			output.append("."sv);
			return output;
		}
	}
	std::string ExpectedMinimumArraySizeMissmatch::formatMessage() const
	{
		auto output = location.format();
		auto expected = to_cardinal_string(m_expected);
		auto got = to_cardinal_string(m_got);

		output.reserve(
			output.length()
			+ "Array was expected to have at least "sv.length()
			+ expected.length()
			+ " elements but has "sv.length()
			+ got.length()
			+ "."sv.length()
		);

		output.append("Array was expected to have at least "sv);
		output.append(expected);
		output.append(" elements but has "sv);
		output.append(got);
		output.append("."sv);
		return output;
	}
	std::string ExpectedMinimumArraySizeMissmatchWeak::formatMessage() const
	{
		auto output = location.format();
		auto expected = to_cardinal_string(m_expected);
		auto got = to_cardinal_string(m_got);

		output.reserve(
			output.length()
			+ "Array was expected to have at least "sv.length()
			+ expected.length()
			+ " elements but has "sv.length()
			+ got.length()
			+ "."sv.length()
		);

		output.append("Array was expected to have at least "sv);
		output.append(expected);
		output.append(" elements but has "sv);
		output.append(got);
		output.append("."sv);
		return output;
	}
	std::string ExpectedArrayTypeMissmatch::formatMessage() const
	{
		std::stringstream sstream;
		auto output = location.format();
		auto position = to_ordinal_string(m_position);
		auto got = ::sqf::type_str(m_got);
		sstream << "Expected the "sv;
		sstream << position;
		sstream << " element of the array to be of the type "sv;
		bool flag = false;
		for (const auto& it : m_expected)
		{
			if (flag)
			{
				sstream << " or ";
			}
			flag = true;
			sstream << ::sqf::type_str(it);
		}
		sstream << " but got "sv;
		sstream << got;
		sstream << "."sv;
	}
	std::string ExpectedArrayTypeMissmatchWeak::formatMessage() const
	{
		std::stringstream sstream;
		auto output = location.format();
		auto position = to_ordinal_string(m_position);
		auto got = ::sqf::type_str(m_got);
		sstream << "Expected the "sv;
		sstream << position;
		sstream << " element of the array to be of the type "sv;
		bool flag = false;
		for (const auto& it : m_expected)
		{
			if (flag)
			{
				sstream << " or ";
			}
			flag = true;
			sstream << ::sqf::type_str(it);
		}
		sstream << " but got "sv;
		sstream << got;
		sstream << "."sv;
	}
	std::string IndexOutOfRange::formatMessage() const
	{
		auto output = location.format();
		auto range = to_cardinal_string(m_range);
		auto index = to_cardinal_string(m_index);

		output.reserve(
			output.length()
			+ "The requested index "sv.length()
			+ index.length()
			+ " exceeds the available range of "sv.length()
			+ range.length()
			+ "."sv.length()
		);

		output.append("The requested index "sv);
		output.append(index);
		output.append(" exceeds the available range of "sv);
		output.append(range);
		output.append("."sv);
		return output;
	}
	std::string IndexOutOfRangeWeak::formatMessage() const
	{
		auto output = location.format();
		auto range = to_cardinal_string(m_range);
		auto index = to_cardinal_string(m_index);

		output.reserve(
			output.length()
			+ "The requested index "sv.length()
			+ index.length()
			+ " exceeds the available range of "sv.length()
			+ range.length()
			+ "."sv.length()
		);

		output.append("The requested index "sv);
		output.append(index);
		output.append(" exceeds the available range of "sv);
		output.append(range);
		output.append("."sv);
		return output;
	}
	std::string NegativeIndex::formatMessage() const
	{
		auto output = location.format();
		const auto message = "The requested index is negative."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NegativeIndexWeak::formatMessage() const
	{
		auto output = location.format();
		const auto message = "The requested index is negative."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string IndexEqualsRange::formatMessage() const
	{
		auto output = location.format();
		auto range = to_cardinal_string(m_range);
		auto index = to_cardinal_string(m_index);

		output.reserve(
			output.length()
			+ "The requested index "sv.length()
			+ index.length()
			+ " is equal to the available range of "sv.length()
			+ range.length()
			+ "."sv.length()
		);

		output.append("The requested index "sv);
		output.append(index);
		output.append(" is equal to the available range of "sv);
		output.append(range);
		output.append("."sv);
		return output;
	}
	std::string ReturningNil::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Returning nil."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ReturningEmptyArray::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Returning empty array."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NegativeSize::formatMessage() const
	{
		auto output = location.format();
		const auto message = "The requested size is negative."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string NegativeSizeWeak::formatMessage() const
	{
		auto output = location.format();
		const auto message = "The requested size is negative."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ArrayRecursion::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Array recursion."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string InfoMessage::formatMessage() const
	{
		auto output = location.format();

		output.reserve(
			output.length()
			+ "["sv.length()
			+ m_source.length()
			+ "] "sv.length()
			+ m_message.length()
		);

		output.append("["sv);
		output.append(m_source);
		output.append("] "sv);
		output.append(m_message);
		return output;
	}
	std::string SuspensionDisabled::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Suspension disabled."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string SuspensionInUnscheduledEnvironment::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Cannot suspend in non-scheduled environment."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ReturningConfigNull::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Returning config null."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string AssertFailed::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Assertion Failed."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string StartIndexExceedsToIndex::formatMessage() const
	{
		auto output = location.format();
		auto from = to_cardinal_string(m_from);
		auto to = to_cardinal_string(m_to);

		output.reserve(
			output.length()
			+ "Start index "sv.length()
			+ from.length()
			+ " exceeds end index "sv.length()
			+ to.length()
			+ "."sv.length()
		);

		output.append("Start index "sv);
		output.append(from);
		output.append(" exceeds end index "sv);
		output.append(to);
		output.append("."sv);
		return output;
	}
	std::string StartIndexExceedsToIndexWeak::formatMessage() const
	{
		auto output = location.format();
		auto from = to_cardinal_string(m_from);
		auto to = to_cardinal_string(m_to);

		output.reserve(
			output.length()
			+ "Start index "sv.length()
			+ from.length()
			+ " exceeds end index "sv.length()
			+ to.length()
			+ "."sv.length()
		);

		output.append("Start index "sv);
		output.append(from);
		output.append(" exceeds end index "sv);
		output.append(to);
		output.append("."sv);
		return output;
	}
	std::string MagicVariableTypeMissmatch::formatMessage() const
	{
		auto output = location.format();
		auto variable_name = m_variable_name;
		auto expected = ::sqf::type_str(m_expected);
		auto got = ::sqf::type_str(m_got);

		output.reserve(
			output.length()
			+ "The Magic variable '"sv.length()
			+ variable_name.length()
			+ "' was expected to be of the type "sv.length()
			+ expected.length()
			+ " but is "sv.length()
			+ got.length()
			+ "."sv.length()
		);

		output.append("The Magic variable '"sv);
		output.append(variable_name);
		output.append("' was expected to be of the type "sv);
		output.append(expected);
		output.append(" but is "sv);
		output.append(got);
		output.append("."sv);
		return output;
	}
	std::string ScriptHandleAlreadyTerminated::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Scripthandle already terminated."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ScriptHandleAlreadyFinished::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Scripthandle already finished."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExtensionLoaded::formatMessage() const
	{
		auto output = location.format();
		auto extension_name = m_extension_name;
		auto version = m_version;

		output.reserve(
			output.length()
			+ "Loaded extension '"sv.length()
			+ extension_name.length()
			+ "' with version "sv.length()
			+ version.length()
			+ "."sv.length()
		);

		output.append("Loaded extension '"sv);
		output.append(extension_name);
		output.append("' with version "sv);
		output.append(version);
		output.append("."sv);
		return output;
	}
	std::string ExtensionNotTerminatingVersionString::formatMessage() const
	{
		auto output = location.format();
		auto extension_name = m_extension_name;

		output.reserve(
			output.length()
			+ "Extension '"sv.length()
			+ extension_name.length()
			+ "' is not terminating the RVExtensionVersion buffer with a '\0'."sv.length()
		);

		output.append("Extension '"sv);
		output.append(extension_name);
		output.append("' is not terminating the RVExtensionVersion buffer with a '\0'."sv);
		return output;
	}
	std::string ExtensionNotTerminatingCallExtensionBufferString::formatMessage() const
	{
		auto output = location.format();
		auto extension_name = m_extension_name;

		output.reserve(
			output.length()
			+ "Extension '"sv.length()
			+ extension_name.length()
			+ "' is not terminating the RVExtension buffer with a '\0'."sv.length()
		);

		output.append("Extension '"sv);
		output.append(extension_name);
		output.append("' is not terminating the RVExtension buffer with a '\0'."sv);
		return output;
	}
	std::string ExtensionNotTerminatingCallExtensionArgBufferString::formatMessage() const
	{
		auto output = location.format();
		auto extension_name = m_extension_name;

		output.reserve(
			output.length()
			+ "Extension '"sv.length()
			+ extension_name.length()
			+ "' is not terminating the RVExtensionArgs buffer with a '\0'."sv.length()
		);

		output.append("Extension '"sv);
		output.append(extension_name);
		output.append("' is not terminating the RVExtensionArgs buffer with a '\0'."sv);
		return output;
	}
	std::string LibraryNameContainsPath::formatMessage() const
	{
		auto output = location.format();
		auto extension_name = m_extension_name;

		output.reserve(
			output.length()
			+ "The extension name '"sv.length()
			+ extension_name.length()
			+ "' is not valid as it contains path characters."sv.length()
		);

		output.append("The extension name '"sv);
		output.append(extension_name);
		output.append("' is not valid as it contains path characters."sv);
		return output;
	}
	std::string ReturningEmptyString::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Returning empty string."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ExtensionRuntimeError::formatMessage() const
	{
		auto output = location.format();
		auto extension_name = m_extension_name;
		auto what = m_what;

		output.reserve(
			output.length()
			+ "Extension '"sv.length()
			+ extension_name.length()
			+ "' caused a runtime exception with the contents '"sv.length()
			+ what.length()
			+ "'."sv.length()
		);

		output.append("Extension '"sv);
		output.append(extension_name);
		output.append("' caused a runtime exception with the contents '"sv);
		output.append(what);
		output.append("'."sv);
		return output;
	}
	std::string FileNotFound::formatMessage() const
	{
		auto output = location.format();
		auto filename = m_filename;

		output.reserve(
			output.length()
			+ "File '"sv.length()
			+ filename.length()
			+ "' could not be located."sv.length()
		);

		output.append("File '"sv);
		output.append(filename);
		output.append("' could not be located."sv);
		return output;
	}
	std::string ScopeNameAlreadySet::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Scropename already set."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ScriptNameAlreadySet::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Scriptname already set."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
	std::string ReturningEmptyScriptHandle::formatMessage() const
	{
		auto output = location.format();
		const auto message = "Returning empty script handle."sv;

		output.reserve(
			output.length()
			+ message.length()
		);

		output.append(message);
		return output;
	}
}