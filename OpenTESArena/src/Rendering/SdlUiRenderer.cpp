#include "SDL.h"

#include "SdlUiRenderer.h"
#include "../Media/TextureBuilder.h"

#include "components/debug/Debug.h"

void SdlUiRenderer::init(SDL_Window *window)
{
	DebugNotImplemented();
}

void SdlUiRenderer::shutdown()
{
	for (auto &pair : this->textures)
	{
		SDL_Texture *texture = pair.second;
		SDL_DestroyTexture(texture);
	}

	this->textures.clear();
}

std::optional<UiTextureID> SdlUiRenderer::tryCreateUiTexture(const TextureBuilder &textureBuilder)
{
	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		DebugNotImplemented();
		return std::nullopt;
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		DebugNotImplemented();
		return std::nullopt;
	}
	else
	{
		DebugUnhandledReturnMsg(std::optional<UiTextureID>,
			std::to_string(static_cast<int>(textureBuilderType)));
	}
}

void SdlUiRenderer::freeUiTexture(UiTextureID id)
{
	DebugNotImplemented();
}

std::optional<Int2> SdlUiRenderer::tryGetTextureDims(UiTextureID id) const
{
	DebugNotImplemented();
	return std::optional<Int2>();
}

void SdlUiRenderer::draw(const RenderElement *elements, int count, RenderSpace renderSpace)
{
	DebugNotImplemented();
}
