#ifndef OPTIONS_UI_VIEW_H
#define OPTIONS_UI_VIEW_H

#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"

namespace OptionsUiView
{
	const Color BackgroundColor(60, 60, 68);

	// Screen locations for various options things.
	const Int2 TabsOrigin(3, 38);
	const Int2 TabsDimensions(54, 16);
	const Int2 ListOrigin(
		TabsOrigin.x + TabsDimensions.x + 5,
		TabsOrigin.y);
	const Int2 ListDimensions(
		254,
		TabsDimensions.y * 5);
	const Int2 DescriptionOrigin(
		TabsOrigin.x + 2,
		TabsOrigin.y + (TabsDimensions.y * 5) + 4);

	const Rect GraphicsTabRect(
		TabsOrigin.x,
		TabsOrigin.y,
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect AudioTabRect(
		TabsOrigin.x,
		TabsOrigin.y + TabsDimensions.y,
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect InputTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 2),
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect MiscTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 3),
		TabsDimensions.x,
		TabsDimensions.y);
	const Rect DevTabRect(
		TabsOrigin.x,
		TabsOrigin.y + (TabsDimensions.y * 4),
		TabsDimensions.x,
		TabsDimensions.y);
}

#endif
