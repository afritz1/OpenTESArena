#ifndef UI_DRAW_CALL_H
#define UI_DRAW_CALL_H

#include <functional>
#include <optional>

#include "../Math/Rect.h"
#include "../Media/TextureUtils.h"

class Texture;

class UiDrawCall
{
public:
	enum class TextureType
	{
		Texture, // SDL texture.
		TextureBuilder // TextureBuilderID + palette ID pair, provided until only using UiTextureID.
	};

	struct TextureInfo
	{
		const Texture *texture;

		TextureInfo(const Texture &texture);
	};

	struct TextureBuilderInfo
	{
		TextureBuilderID textureBuilderID;
		PaletteID paletteID;

		TextureBuilderInfo(TextureBuilderID textureBuilderID, PaletteID paletteID);
	};

	using TextureFunc = std::function<TextureInfo()>;
	using TextureBuilderFunc = std::function<TextureBuilderInfo()>;
	using RectFunc = std::function<Rect()>;
	using ActiveFunc = std::function<bool()>;
private:
	TextureType textureType; // Determines which texture function to use.
	TextureFunc textureFunc;
	TextureBuilderFunc textureBuilderFunc;

	RectFunc rectFunc; // Position + size on-screen.
	ActiveFunc activeFunc; // Whether to attempt to draw.
	std::optional<Rect> clipRect; // For drawing within a clipped area.

	void init(TextureType textureType, const RectFunc &rectFunc, const ActiveFunc &activeFunc,
		const std::optional<Rect> &clipRect);
public:
	UiDrawCall();

	static TextureBuilderFunc makeTextureBuilderFunc(TextureBuilderID textureBuilderID, PaletteID paletteID);
	static bool defaultActiveFunc();
	static RectFunc makeRectFunc(const Rect &rect);

	void initWithTexture(const TextureFunc &textureFunc, const RectFunc &rectFunc,
		const ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);
	void initWithTextureBuilder(const TextureBuilderFunc &textureBuilderFunc, const RectFunc &rectFunc,
		const ActiveFunc &activeFunc, const std::optional<Rect> &clipRect = std::nullopt);

	TextureType getTextureType() const;
	TextureInfo getTextureInfo() const;
	TextureBuilderInfo getTextureBuilderInfo() const;

	Rect getRect() const;
	bool isActive() const;
	const std::optional<Rect> &getClipRect() const;
};

#endif
