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
		DebugLogErrorFormat("Couldn't get palette ID for \"%s\".", animFilename.c_str());
		return Buffer<UiTextureID>();
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	const std::optional<TextureBuilderIdGroup> textureBuilderIDs = textureManager.tryGetTextureBuilderIDs(animFilename.c_str());
	if (!textureBuilderIDs.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture builder IDs for \"%s\".", animFilename.c_str());
		return Buffer<UiTextureID>();
	}

	const int textureCount = textureBuilderIDs->count;
	Buffer<UiTextureID> textureIDs(textureCount);
	for (int i = 0; i < textureCount; i++)
	{
		const TextureBuilderID textureBuilderID = textureBuilderIDs->getID(i);
		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);
		const UiTextureID textureID = renderer.createUiTexture(textureBuilder.width, textureBuilder.height);
		if (textureID < 0)
		{
			DebugLogErrorFormat("Couldn't create UI texture for \"%s\" index %d.", animFilename.c_str(), i);
		}

		textureIDs.set(i, textureID);

		if (!renderer.populateUiTexture(textureID, textureBuilder.bytes, &palette))
		{
			DebugLogErrorFormat("Couldn't populate UI texture for \"%s\" index %d.", animFilename.c_str(), i);
		}
	}

	return textureIDs;
}
