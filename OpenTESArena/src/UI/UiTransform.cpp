#include "PivotType.h"
#include "UiTransform.h"

UiTransform::UiTransform()
{
	this->pivotType = PivotType::TopLeft;
	this->parentInstID = -1;
}

void UiTransform::init(Int2 position, Int2 size, PivotType pivotType)
{
	this->position = position;
	this->size = size;
	this->pivotType = pivotType;
}
