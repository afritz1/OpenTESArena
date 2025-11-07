#ifndef UI_TRANSFORM_H
#define UI_TRANSFORM_H

#include "../Math/Vector2.h"

enum class PivotType;

// Determines logical dimensions for the layout and rendering of an element.
enum class UiTransformSizeType
{
	Content, // Driven by the element's image, text box, etc..
	Manual // Driven by transform size which may stretch/squish the image/text box/etc..
};

using UiTransformInstanceID = int;

struct UiTransform
{
	Int2 position;
	Int2 size; // Only used if size type is manual.
	UiTransformSizeType sizeType;
	PivotType pivotType;
	UiTransformInstanceID parentInstID;

	UiTransform();

	void init(Int2 position, Int2 size, UiTransformSizeType sizeType, PivotType pivotType);
};

#endif
