#include "UiPivotType.h"
#include "UiTransform.h"

UiTransform::UiTransform()
{
	this->sizeType = UiTransformSizeType::Content;
	this->pivotType = UiPivotType::TopLeft;
	this->parentInstID = -1;
}

void UiTransform::init(Int2 position, Int2 size, UiTransformSizeType sizeType, UiPivotType pivotType)
{
	this->position = position;
	this->size = size;
	this->sizeType = sizeType;
	this->pivotType = pivotType;
	this->parentInstID = -1;
}
