#ifndef UI_DRAW_CALL_H
#define UI_DRAW_CALL_H

#include <functional>
#include <optional>

#include "../Math/Rect.h"
#include "../Rendering/RenderTextureUtils.h"

class UiDrawCall
{
public:
	using TextureFunc = std::function<UiTextureID()>;
	using RectFunc = std::function<Rect()>;
	using ActiveFunc = std::function<bool()>;
private:
	TextureFunc textureFunc; // UI texture to render with.
	RectFunc rectFunc; // Position + size on-screen.
	ActiveFunc activeFunc; // Whether to attempt to draw.
	std::optional<Rect> clipRect; // For drawing within a clipped area.
public:
	UiDrawCall(const TextureFunc &textureFunc, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
		const std::optional<Rect> &clipRect = std::nullopt);
	UiDrawCall(const TextureFunc &textureFunc, const RectFunc &rectFunc, const std::optional<Rect> &clipRect = std::nullopt);
	UiDrawCall(const TextureFunc &textureFunc, const Rect &rect, const ActiveFunc &activeFunc,
		const std::optional<Rect> &clipRect = std::nullopt);
	UiDrawCall(const TextureFunc &textureFunc, const Rect &rect, const std::optional<Rect> &clipRect = std::nullopt);
	UiDrawCall(UiTextureID textureID, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
		const std::optional<Rect> &clipRect = std::nullopt);
	UiDrawCall(UiTextureID textureID, const RectFunc &rectFunc, const std::optional<Rect> &clipRect = std::nullopt);
	UiDrawCall(UiTextureID textureID, const Rect &rect, const ActiveFunc &activeFunc,
		const std::optional<Rect> &clipRect = std::nullopt);
	UiDrawCall(UiTextureID textureID, const Rect &rect, const std::optional<Rect> &clipRect = std::nullopt);

	UiTextureID getTextureID() const;
	Rect getRect() const;
	bool isActive() const;
	const std::optional<Rect> &getClipRect() const;
};

#endif
