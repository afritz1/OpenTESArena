#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "HexPrinter.h"
#include "String.h"

namespace
{
	constexpr char DIVIDER = '|';
	constexpr char NON_PRINTABLE_CHAR = '.';

	constexpr int BYTES_PER_LINE = 16;
	constexpr int SPACING = 1;
	constexpr int ADDRESS_CHAR_COUNT = 8;
	constexpr int LINE_SIZE = 2 + ADDRESS_CHAR_COUNT + (SPACING * 3) + (BYTES_PER_LINE * 3) +
		(SPACING * 2) + BYTES_PER_LINE;

	bool IsPrintableChar(char c)
	{
		return (c >= 32) && (c < 127);
	}

	char NibbleToHex(int n)
	{
		return (n < 0xA) ? ('0' + n) : ('A' + (n - 0xA));
	}

	std::string MakeFormattedLine(int address, const uint8_t *data, int count)
	{
		std::string line(LINE_SIZE, '\0');

		const std::string addressHexStr = [address]()
		{
			std::stringstream addressHex;
			addressHex << std::hex << std::setfill('0') << std::setw(ADDRESS_CHAR_COUNT) << address;
			return addressHex.str();
		}();

		line[0] = '0';
		line[1] = 'x';
		std::copy(addressHexStr.begin(), addressHexStr.end(), line.begin() + 2);

		const int addressEndIndex = static_cast<int>(addressHexStr.size()) + 2;
		line[addressEndIndex] = ' ';
		line[addressEndIndex + 1] = DIVIDER;
		line[addressEndIndex + 2] = ' ';

		const int dataStartIndex = addressEndIndex + 3;
		std::string byteHexStr(2, '\0');
		for (int i = 0; i < BYTES_PER_LINE; i++)
		{
			if (i < count)
			{
				byteHexStr[0] = NibbleToHex(data[i] >> 4);
				byteHexStr[1] = NibbleToHex(data[i] & 0xF);
			}
			else
			{
				byteHexStr[0] = ' ';
				byteHexStr[1] = ' ';
			}

			const int index = dataStartIndex + (i * 3);
			line[index] = byteHexStr[0];
			line[index + 1] = byteHexStr[1];
			line[index + 2] = ' ';
		}

		const int endDividerIndex = dataStartIndex + (BYTES_PER_LINE * 3);
		line[endDividerIndex] = DIVIDER;
		line[endDividerIndex + 1] = ' ';

		const int literalByteStartIndex = endDividerIndex + 2;
		for (int i = 0; i < count; i++)
		{
			const int index = literalByteStartIndex + i;
			const char c = static_cast<char>(data[i]);
			line[index] = IsPrintableChar(c) ? c : NON_PRINTABLE_CHAR;
		}

		return line;
	}
}

void HexPrinter::print(const uint8_t *data, int count, const char *filename)
{
	const int lineCount = (count + (BYTES_PER_LINE - 1)) / BYTES_PER_LINE;

	if (filename != nullptr)
	{
		std::ofstream ofs(filename, std::ios::binary);

		for (int i = 0; i < lineCount; i++)
		{
			const int address = BYTES_PER_LINE * i;
			const uint8_t *ptr = data + address;
			const int countOnLine = std::min(BYTES_PER_LINE, count - address);

			std::string line = MakeFormattedLine(address, ptr, countOnLine);
			ofs << line;
			ofs << '\n';
		}
	}
	else
	{
		for (int i = 0; i < lineCount; i++)
		{
			const int address = BYTES_PER_LINE * i;
			const uint8_t *ptr = data + address;
			const int countOnLine = std::min(BYTES_PER_LINE, count - address);

			std::string line = MakeFormattedLine(address, ptr, countOnLine);
			std::cout << line << '\n';
		}
	}
}

void HexPrinter::print(const uint8_t *data, int count)
{
	HexPrinter::print(data, count, nullptr);
}
