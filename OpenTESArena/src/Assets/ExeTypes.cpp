#include "ExeTypes.h"

#include "../Utilities/Bytes.h"

const size_t ExeTypes::Rect16::SIZE = sizeof(int16_t) * 4;

void ExeTypes::Rect16::init(const char *data)
{
	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);
	this->x = Bytes::getLE16(ptr);
	this->y = Bytes::getLE16(ptr + sizeof(int16_t));
	this->w = Bytes::getLE16(ptr + (sizeof(int16_t) * 2));
	this->h = Bytes::getLE16(ptr + (sizeof(int16_t) * 3));
}

const uint16_t ExeTypes::List::ALIGNMENT_MASK = 0x3000;
const uint16_t ExeTypes::List::LEFT_ALIGNMENT = 0x0000;
const uint16_t ExeTypes::List::RIGHT_ALIGNMENT = 0x1000;
const uint16_t ExeTypes::List::CENTER_ALIGNMENT = 0x2000;

void ExeTypes::List::init(const char *data)
{
	this->buttonUp.init(data);
	this->buttonDown.init(data + ExeTypes::Rect16::SIZE);
	this->scrollBar.init(data + (ExeTypes::Rect16::SIZE * 2));
	this->area.init(data + (ExeTypes::Rect16::SIZE * 3));

	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);
	this->flags = Bytes::getLE16(ptr + (ExeTypes::Rect16::SIZE * 4));
}
