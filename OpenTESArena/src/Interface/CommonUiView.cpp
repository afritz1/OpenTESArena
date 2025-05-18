#include "CommonUiView.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAsset.h"
#include "../Assets/TextureManager.h"
#include "../Rendering/Renderer.h"

#include "components/debug/Debug.h"

UiTextureID CommonUiView::allocDefaultCursorTexture(TextureManager &textureManager, Renderer &renderer)
{
	const TextureAsset paletteTextureAsset = TextureAsset(std::string(ArenaPaletteName::Default));
	const TextureAsset textureAsset = TextureAsset(std::string(ArenaTextureName::SwordCursor));

	UiTextureID textureID;
	if (!TextureUtils::tryAllocUiTexture(textureAsset, paletteTextureAsset, textureManager, renderer, &textureID))
	{
		DebugCrash("Couldn't create UI texture for default cursor.");
	}

	return textureID;
}

Color CommonUiView::getDebugInfoTextBoxColor()
{
	return Colors::White;
}

Rect CommonUiView::getDebugInfoTextBoxRect()
{
	return Rect(2, 2, 200, 150);
}

TextBoxInitInfo CommonUiView::getDebugInfoTextBoxInitInfo(const FontLibrary &fontLibrary)
{
	std::string dummyText;
	for (int i = 0; i < 18; i++)
	{
		if (dummyText.length() > 0)
		{
			dummyText += '\n';
		}

		dummyText += std::string(30, TextRenderUtils::LARGEST_CHAR);
	}

	const TextRenderUtils::TextShadowInfo shadowInfo(1, 1, Colors::Black);

	const Rect rect = CommonUiView::getDebugInfoTextBoxRect();
	return TextBoxInitInfo::makeWithXY(
		dummyText,
		rect.getLeft(),
		rect.getTop(),
		CommonUiView::DebugInfoFontName,
		CommonUiView::getDebugInfoTextBoxColor(),
		CommonUiView::DebugInfoTextAlignment,
		shadowInfo,
		0,
		fontLibrary);
}
