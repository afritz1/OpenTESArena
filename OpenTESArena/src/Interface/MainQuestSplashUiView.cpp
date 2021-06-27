#include "MainQuestSplashUiView.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontUtils.h"

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
	const char *fontNameStr = FontUtils::fromName(MainQuestSplashUiView::DescriptionFontName);
	int fontDefIndex;
	if (!fontLibrary.tryGetDefinitionIndex(fontNameStr, &fontDefIndex))
	{
		DebugCrash("Couldn't get font definition for \"" + std::string(fontNameStr) + "\".");
	}

	const FontDefinition &fontDef = fontLibrary.getDefinition(fontDefIndex);
	const TextRenderUtils::TextureGenInfo textureGenInfo = TextRenderUtils::makeTextureGenInfo(text, fontDef);
	return TextBox::InitInfo::makeWithXY(
		text,
		MainQuestSplashUiView::getDescriptionTextBoxX(textureGenInfo.width),
		MainQuestSplashUiView::getDescriptionTextBoxY(),
		MainQuestSplashUiView::DescriptionFontName,
		MainQuestSplashUiView::DescriptionTextColor,
		MainQuestSplashUiView::DescriptionTextAlignment,
		std::nullopt,
		MainQuestSplashUiView::DescriptionLineSpacing,
		fontLibrary);
}
