#include "UiDrawCall.h"

#include "components/debug/Debug.h"

UiDrawCallInitInfo::UiDrawCallInitInfo()
{
	this->textureID = -1;
	this->pivotType = PivotType::TopLeft;
	this->activeFunc = UiDrawCall::defaultActiveFunc;
	this->renderSpace = UiRenderSpace::Classic;
}

UiDrawCall::UiDrawCall(const UiDrawCallTextureFunc &textureFunc, const UiDrawCallPositionFunc &positionFunc, const UiDrawCallSizeFunc &sizeFunc,
	const UiDrawCallPivotFunc &pivotFunc, const UiDrawCallActiveFunc &activeFunc, const std::optional<Rect> &clipRect, UiRenderSpace renderSpace)
	: textureFunc(textureFunc), positionFunc(positionFunc), sizeFunc(sizeFunc), pivotFunc(pivotFunc), activeFunc(activeFunc), clipRect(clipRect)
{
	DebugAssert(this->textureFunc);
	DebugAssert(this->positionFunc);
	DebugAssert(this->sizeFunc);
	DebugAssert(this->pivotFunc);
	DebugAssert(this->activeFunc);
	this->renderSpace = renderSpace;
}

UiDrawCall::UiDrawCall(const UiDrawCallInitInfo &initInfo)
{
	if (initInfo.textureFunc)
	{
		DebugAssert(initInfo.textureID < 0);
		this->textureFunc = initInfo.textureFunc;
	}
	else
	{
		DebugAssert(initInfo.textureID >= 0);
		this->textureFunc = UiDrawCall::makeTextureFunc(initInfo.textureID);
	}

	if (initInfo.positionFunc)
	{
		DebugAssert(initInfo.position == Int2::Zero);
		this->positionFunc = initInfo.positionFunc;
	}
	else
	{
		this->positionFunc = UiDrawCall::makePositionFunc(initInfo.position);
	}

	if (initInfo.sizeFunc)
	{
		DebugAssert(initInfo.size == Int2::Zero);
		this->sizeFunc = initInfo.sizeFunc;
	}
	else
	{
		DebugAssert(initInfo.size.x > 0 && initInfo.size.y > 0);
		this->sizeFunc = UiDrawCall::makeSizeFunc(initInfo.size);
	}

	if (initInfo.pivotFunc)
	{
		DebugAssert(initInfo.pivotType == PivotType::TopLeft);
		this->pivotFunc = initInfo.pivotFunc;
	}
	else
	{
		this->pivotFunc = UiDrawCall::makePivotFunc(initInfo.pivotType);
	}

	this->activeFunc = initInfo.activeFunc;
	this->clipRect = initInfo.clipRect;
	this->renderSpace = initInfo.renderSpace;
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
