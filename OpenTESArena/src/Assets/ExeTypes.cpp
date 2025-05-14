#include "ExeTypes.h"

#include "components/utilities/Bytes.h"

void ExeTypes::Rect16::init(const std::byte *data)
{
	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);
	this->x = Bytes::getLE16(ptr);
	this->y = Bytes::getLE16(ptr + sizeof(int16_t));
	this->w = Bytes::getLE16(ptr + (sizeof(int16_t) * 2));
	this->h = Bytes::getLE16(ptr + (sizeof(int16_t) * 3));
}

void ExeTypes::List::init(const std::byte *data)
{
	this->buttonUp.init(data);
	this->buttonDown.init(data + ExeTypes::Rect16::SIZE);
	this->scrollBar.init(data + (ExeTypes::Rect16::SIZE * 2));
	this->area.init(data + (ExeTypes::Rect16::SIZE * 3));

	const uint8_t *ptr = reinterpret_cast<const uint8_t*>(data);
	this->flags = Bytes::getLE16(ptr + (ExeTypes::Rect16::SIZE * 4));
}
