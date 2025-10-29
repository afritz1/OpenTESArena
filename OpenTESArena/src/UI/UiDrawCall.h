#ifndef UI_DRAW_CALL_H
#define UI_DRAW_CALL_H

#include <functional>
#include <optional>

#include "PivotType.h"
#include "UiRenderSpace.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"

using UiDrawCallTextureFunc = std::function<UiTextureID()>;
using UiDrawCallPositionFunc = std::function<Int2()>;
using UiDrawCallSizeFunc = std::function<Int2()>;
using UiDrawCallPivotFunc = std::function<PivotType()>;
using UiDrawCallActiveFunc = std::function<bool()>;

struct UiDrawCallInitInfo
{
	UiDrawCallTextureFunc textureFunc;
	UiTextureID textureID;

	UiDrawCallPositionFunc positionFunc;
	Int2 position;

	UiDrawCallSizeFunc sizeFunc;
	Int2 size;

	UiDrawCallPivotFunc pivotFunc;
	PivotType pivotType;

	UiDrawCallActiveFunc activeFunc;

	std::optional<Rect> clipRect;

	UiRenderSpace renderSpace;

	UiDrawCallInitInfo();
};

struct UiDrawCall
{
	UiDrawCallTextureFunc textureFunc; // UI texture to render with.
	UiDrawCallPositionFunc positionFunc; // On-screen position.
	UiDrawCallSizeFunc sizeFunc; // Width + height in pixels.
	UiDrawCallPivotFunc pivotFunc; // Affects how the dimensions expand from the position (for UI scaling).
	UiDrawCallActiveFunc activeFunc; // Whether to attempt to draw.
	std::optional<Rect> clipRect; // For drawing within a clipped area in the selected render space.
	UiRenderSpace renderSpace; // Relative positioning and sizing in the application window.

	UiDrawCall(const UiDrawCallTextureFunc &textureFunc, const UiDrawCallPositionFunc &positionFunc, const UiDrawCallSizeFunc &sizeFunc,
		const UiDrawCallPivotFunc &pivotFunc, const UiDrawCallActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt,
		UiRenderSpace renderSpace = UiRenderSpace::Classic);
	UiDrawCall(const UiDrawCallInitInfo &initInfo);

	static UiDrawCallTextureFunc makeTextureFunc(UiTextureID id);
	static UiDrawCallPositionFunc makePositionFunc(const Int2 &position);
	static UiDrawCallSizeFunc makeSizeFunc(const Int2 &size);
	static UiDrawCallPivotFunc makePivotFunc(PivotType pivotType);
	static bool defaultActiveFunc();
};

#endif
