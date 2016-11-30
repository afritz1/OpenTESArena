#include "RectData.h"

RectData::RectData(const Rect3D &rect, int textureID)
	: rect(rect)
{
	this->textureID = textureID;
}

RectData::~RectData()
{

}

const Rect3D &RectData::getRect() const
{
	return this->rect;
}

int RectData::getTextureID() const
{
	return this->textureID;
}
