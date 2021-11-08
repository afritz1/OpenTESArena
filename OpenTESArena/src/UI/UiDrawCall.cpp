#include "UiDrawCall.h"

#include "components/debug/Debug.h"

UiDrawCall::UiDrawCall(const TextureFunc &textureFunc, const PositionFunc &positionFunc, const SizeFunc &sizeFunc,
	const PivotFunc &pivotFunc, const ActiveFunc &activeFunc, const std::optional<Rect> &clipRect,
	RenderSpace renderSpace)
	: textureFunc(textureFunc), positionFunc(positionFunc), sizeFunc(sizeFunc), pivotFunc(pivotFunc),
	activeFunc(activeFunc), clipRect(clipRect)
{
	DebugAssert(this->textureFunc);
	DebugAssert(this->positionFunc);
	DebugAssert(this->sizeFunc);
	DebugAssert(this->pivotFunc);
	DebugAssert(this->activeFunc);
	this->renderSpace = renderSpace;
}

UiDrawCall::TextureFunc UiDrawCall::makeTextureFunc(UiTextureID id)
{
	return [id]() { return id; };
}

UiDrawCall::PositionFunc UiDrawCall::makePositionFunc(const Int2 &position)
{
	return [position]() { return position; };
}

UiDrawCall::SizeFunc UiDrawCall::makeSizeFunc(const Int2 &size)
{
	return [size]() { return size; };
}

UiDrawCall::PivotFunc UiDrawCall::makePivotFunc(PivotType pivotType)
{
	return [pivotType]() { return pivotType; };
}

bool UiDrawCall::defaultActiveFunc()
{
	return true;
}

UiTextureID UiDrawCall::getTextureID() const
{
	DebugAssert(this->isActive());
	return this->textureFunc();
}

Int2 UiDrawCall::getPosition() const
{
	DebugAssert(this->isActive());
	return this->positionFunc();
}

Int2 UiDrawCall::getSize() const
{
	DebugAssert(this->isActive());
	return this->sizeFunc();
}

PivotType UiDrawCall::getPivotType() const
{
	DebugAssert(this->isActive());
	return this->pivotFunc();
}

bool UiDrawCall::isActive() const
{
	return this->activeFunc();
}

const std::optional<Rect> &UiDrawCall::getClipRect() const
{
	DebugAssert(this->isActive());
	return this->clipRect;
}

RenderSpace UiDrawCall::getRenderSpace() const
{
	DebugAssert(this->isActive());
	return this->renderSpace;
}
