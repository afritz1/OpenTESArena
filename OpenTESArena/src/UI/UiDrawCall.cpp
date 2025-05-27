#include "UiDrawCall.h"

#include "components/debug/Debug.h"

UiDrawCall::UiDrawCall(const UiDrawCallTextureFunc &textureFunc, const UiDrawCallPositionFunc &positionFunc, const UiDrawCallSizeFunc &sizeFunc,
	const UiDrawCallPivotFunc &pivotFunc, const UiDrawCallActiveFunc &activeFunc, const std::optional<Rect> &clipRect, RenderSpace renderSpace)
	: textureFunc(textureFunc), positionFunc(positionFunc), sizeFunc(sizeFunc), pivotFunc(pivotFunc), activeFunc(activeFunc), clipRect(clipRect)
{
	DebugAssert(this->textureFunc);
	DebugAssert(this->positionFunc);
	DebugAssert(this->sizeFunc);
	DebugAssert(this->pivotFunc);
	DebugAssert(this->activeFunc);
	this->renderSpace = renderSpace;
}

UiDrawCallTextureFunc UiDrawCall::makeTextureFunc(UiTextureID id)
{
	return [id]() { return id; };
}

UiDrawCallPositionFunc UiDrawCall::makePositionFunc(const Int2 &position)
{
	return [position]() { return position; };
}

UiDrawCallSizeFunc UiDrawCall::makeSizeFunc(const Int2 &size)
{
	return [size]() { return size; };
}

UiDrawCallPivotFunc UiDrawCall::makePivotFunc(PivotType pivotType)
{
	return [pivotType]() { return pivotType; };
}

bool UiDrawCall::defaultActiveFunc()
{
	return true;
}
