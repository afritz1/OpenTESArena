#include "IntroUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAssetReference.h"

std::string IntroUiView::getIntroBookSequenceFilename()
{
	return ArenaTextureSequenceName::IntroBook;
}

std::string IntroUiView::getIntroBookPaletteFilename()
{
	return ArenaTextureSequenceName::IntroBook;
}

TextureAssetReference IntroUiView::getIntroTitleTextureAssetReference()
{
	return TextureAssetReference(std::string(ArenaTextureName::IntroTitle));
}

TextureAssetReference IntroUiView::getIntroTitlePaletteTextureAssetReference()
{
	return TextureAssetReference(std::string(ArenaTextureName::IntroTitle));
}

TextureAssetReference IntroUiView::getIntroQuoteTextureAssetReference()
{
	return TextureAssetReference(std::string(ArenaTextureName::IntroQuote));
}

TextureAssetReference IntroUiView::getIntroQuotePaletteTextureAssetReference()
{
	return TextureAssetReference(std::string(ArenaTextureName::IntroQuote));
}

std::string IntroUiView::getOpeningScrollSequenceFilename()
{
	return ArenaTextureSequenceName::OpeningScroll;
}

std::string IntroUiView::getOpeningScrollPaletteFilename()
{
	return ArenaTextureSequenceName::OpeningScroll;
}

std::vector<std::string> IntroUiView::getIntroStoryTextureNames()
{
	return { "SCROLL01.IMG", "SCROLL02.IMG", "SCROLL03.IMG" };
}

std::vector<std::string> IntroUiView::getIntroStoryPaletteNames()
{
	return { "SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG" };
}

std::vector<double> IntroUiView::getIntroStoryImageDurations()
{
	// Last frame is slightly shorter.
	return { 13.0, 13.0, 10.0 };
}
