#include "ExeTypes.h"

#include "components/utilities/Bytes.h"

void ExeTypes::Rect16::init(Span<const std::byte> exeBytes, int exeAddress)
{
	DebugAssert(exeBytes.isValidRange(exeAddress, ExeTypes::Rect16::SIZE));
	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(exeBytes.begin()) + exeAddress;
	this->x = Bytes::getLE16(ptr);
	this->y = Bytes::getLE16(ptr + sizeof(int16_t));
	this->w = Bytes::getLE16(ptr + (sizeof(int16_t) * 2));
	this->h = Bytes::getLE16(ptr + (sizeof(int16_t) * 3));
}

void ExeTypes::List::init(Span<const std::byte> exeBytes, int exeAddress)
{
	DebugAssert(exeBytes.isValidRange(exeAddress, ExeTypes::List::SIZE));
	this->buttonUp.init(exeBytes, exeAddress);
	this->buttonDown.init(exeBytes, exeAddress + ExeTypes::Rect16::SIZE);
	this->scrollBar.init(exeBytes, exeAddress + (ExeTypes::Rect16::SIZE * 2));
	this->area.init(exeBytes, exeAddress + (ExeTypes::Rect16::SIZE * 3));

	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(exeBytes.begin()) + exeAddress;
	this->flags = Bytes::getLE16(ptr + (ExeTypes::Rect16::SIZE * 4));
}
