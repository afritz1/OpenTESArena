#include "SDL.h"

#include "SdlUiRenderer.h"
#include "../Assets/TextureAssetReference.h"
#include "../Media/TextureBuilder.h"
#include "../Media/TextureManager.h"

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

bool SdlUiRenderer::tryCreateUiTexture(const TextureAssetReference &textureAssetRef,
	TextureManager &textureManager)
{
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get UI texture builder ID for \"" + textureAssetRef.filename + "\".");
		return false;
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		DebugNotImplemented();
		return true;
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		DebugNotImplemented();
		return true;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}
}

void SdlUiRenderer::freeUiTexture(const TextureAssetReference &textureAssetRef)
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
