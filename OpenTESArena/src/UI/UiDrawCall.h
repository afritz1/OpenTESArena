#ifndef UI_DRAW_CALL_H
#define UI_DRAW_CALL_H

#include <functional>
#include <optional>

#include "PivotType.h"
#include "RenderSpace.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"

class UiDrawCall
{
public:
	using TextureFunc = std::function<UiTextureID()>;
	using PositionFunc = std::function<Int2()>;
	using SizeFunc = std::function<Int2()>;
	using PivotFunc = std::function<PivotType()>;
	using ActiveFunc = std::function<bool()>;
private:
	TextureFunc textureFunc; // UI texture to render with.
	PositionFunc positionFunc; // On-screen position.
	SizeFunc sizeFunc; // Width + height in pixels.
	PivotFunc pivotFunc; // Affects how the dimensions expand from the position (for UI scaling).
	ActiveFunc activeFunc; // Whether to attempt to draw.
	std::optional<Rect> clipRect; // For drawing within a clipped area in the selected render space.
	RenderSpace renderSpace; // Relative positioning and sizing in the application window.
public:
	UiDrawCall(const TextureFunc &textureFunc, const PositionFunc &positionFunc, const SizeFunc &sizeFunc,
		const PivotFunc &pivotFunc, const ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt,
		RenderSpace renderSpace = RenderSpace::Classic);

	static TextureFunc makeTextureFunc(UiTextureID id);
	static PositionFunc makePositionFunc(const Int2 &position);
	static SizeFunc makeSizeFunc(const Int2 &size);
	static PivotFunc makePivotFunc(PivotType pivotType);
	static bool defaultActiveFunc();

	UiTextureID getTextureID() const;
	Int2 getPosition() const;
	Int2 getSize() const;
	PivotType getPivotType() const;
	bool isActive() const;
	const std::optional<Rect> &getClipRect() const;
	RenderSpace getRenderSpace() const;
};

#endif
