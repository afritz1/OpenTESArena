#ifndef UI_PIVOT_TYPE_H
#define UI_PIVOT_TYPE_H

// UI pivot types which change the origin within a rect. For example, if the pivot is TopLeft,
// then the rect's dimensions expand to the right and down from the top left corner.
enum class UiPivotType
{
	TopLeft,
	Top,
	TopRight,
	MiddleLeft,
	Middle,
	MiddleRight,
	BottomLeft,
	Bottom,
	BottomRight
};

#endif
