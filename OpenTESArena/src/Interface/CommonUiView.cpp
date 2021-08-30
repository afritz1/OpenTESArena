#include <optional>

#include "CommonUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiTextureID CommonUiView::allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer)
{
	const std::string &paletteFilename = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
	}

	const std::string &textureFilename = ArenaTextureName::SwordCursor;
	const std::optional<TextureBuilderID> textureBuilderID =
		textureManager.tryGetTextureBuilderID(textureFilename.c_str());
	if (!textureBuilderID.has_value())
	{
		DebugCrash("Couldn't get texture builder ID for \"" + textureFilename + "\".");
	}

	UiTextureID textureID;
	if (!renderer.tryCreateUiTexture(*textureBuilderID, *paletteID, textureManager, &textureID))
	{
		DebugCrash("Couldn't create UI texture for default cursor.");
	}

	return textureID;
}
