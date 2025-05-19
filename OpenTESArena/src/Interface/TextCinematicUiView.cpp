#include "TextCinematicUiView.h"
#include "../Assets/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../UI/ArenaFontName.h"

std::string TextCinematicUiView::getSubtitleTextBoxFontName()
{
	return ArenaFontName::Arena;
}

TextBoxInitInfo TextCinematicUiView::getSubtitlesTextBoxInitInfo(const Color &fontColor, const FontLibrary &fontLibrary)
{
	std::string dummyText;
	for (int i = 0; i < 3; i++)
	{
		if (dummyText.length() > 0)
		{
			dummyText += '\n';
		}

		dummyText += std::string(36, TextRenderUtils::LARGEST_CHAR);
	}

	return TextBoxInitInfo::makeWithCenter(
		dummyText,
		TextCinematicUiView::SubtitleTextBoxCenterPoint,
		TextCinematicUiView::getSubtitleTextBoxFontName(),
		fontColor,
		TextCinematicUiView::SubtitleTextBoxTextAlignment,
		std::nullopt,
		TextCinematicUiView::SubtitleTextBoxLineSpacing,
		fontLibrary);
}

Buffer<UiTextureID> TextCinematicUiView::allocAnimationTextures(const std::string &animFilename,
	TextureManager &textureManager, Renderer &renderer)
{
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(animFilename.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette ID for \"" + animFilename + "\".");
	}

	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(animFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugCrash("Couldn't get texture builder IDs for \"" + animFilename + "\".");
	}

	const int textureCount = textureBuilderIDs->getCount();
	Buffer<UiTextureID> textureIDs(textureCount);
	for (int i = 0; i < textureCount; i++)
	{
		const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(i);
		UiTextureID textureID;
		if (!renderer.tryCreateUiTexture(textureBuilderID, *paletteID, textureManager, &textureID))
		{
			DebugCrash("Couldn't create UI texture for \"" + animFilename + "\" index " + std::to_string(i) + ".");
		}

		textureIDs.set(i, textureID);
	}

	return textureIDs;
}
