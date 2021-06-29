#include "MainQuestSplashUiView.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontLibrary.h"

int MainQuestSplashUiView::getDescriptionTextBoxX(int textWidth)
{
	return (ArenaRenderUtils::SCREEN_WIDTH / 2) - (textWidth / 2);
}

int MainQuestSplashUiView::getDescriptionTextBoxY()
{
	return 133;
}

TextBox::InitInfo MainQuestSplashUiView::getDescriptionTextBoxInitInfo(const std::string_view &text, const FontLibrary &fontLibrary)
{
	const std::string &fontName = MainQuestSplashUiView::DescriptionFontName;
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontName.c_str(), &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + fontName + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return TextBox::InitInfo::makeWithXY(
		text,
		MainQuestSplashUiView::getDescriptionTextBoxX(textureGenInfo.width),
		MainQuestSplashUiView::getDescriptionTextBoxY(),
		fontName,
		MainQuestSplashUiView::DescriptionTextColor,
		MainQuestSplashUiView::DescriptionTextAlignment,
		std::nullopt,
		MainQuestSplashUiView::DescriptionLineSpacing,
		fontLibrary);
}
