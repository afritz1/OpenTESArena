#ifndef UI_TRANSFORM_H
#define UI_TRANSFORM_H

#include "../Math/Vector2.h"

enum class PivotType;

using UiTransformInstanceID = int;

struct UiTransform
{
	Int2 position;
	Int2 size;
	PivotType pivotType;
	UiTransformInstanceID parentInstID;

	UiTransform();

	void init(Int2 position, Int2 size, PivotType pivotType);
};

#endif
