#include "IntroUiView.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/TextureAsset.h"

std::string IntroUiView::getIntroBookSequenceFilename()
{
	return ArenaTextureSequenceName::IntroBook;
}

std::string IntroUiView::getIntroBookPaletteFilename()
{
	return ArenaTextureSequenceName::IntroBook;
}

TextureAsset IntroUiView::getIntroTitleTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::IntroTitle));
}

TextureAsset IntroUiView::getIntroTitlePaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::IntroTitle));
}

TextureAsset IntroUiView::getIntroQuoteTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::IntroQuote));
}

TextureAsset IntroUiView::getIntroQuotePaletteTextureAsset()
{
	return TextureAsset(std::string(ArenaTextureName::IntroQuote));
}

std::string IntroUiView::getOpeningScrollSequenceFilename()
{
	return ArenaTextureSequenceName::OpeningScroll;
}

std::string IntroUiView::getOpeningScrollPaletteFilename()
{
	return ArenaTextureSequenceName::OpeningScroll;
}

Buffer<std::string> IntroUiView::getIntroStoryTextureNames()
{
	return { "SCROLL01.IMG", "SCROLL02.IMG", "SCROLL03.IMG" };
}

Buffer<std::string> IntroUiView::getIntroStoryPaletteNames()
{
	return { "SCROLL03.IMG", "SCROLL03.IMG", "SCROLL03.IMG" };
}

Buffer<double> IntroUiView::getIntroStoryImageDurations()
{
	// Last frame is slightly shorter.
	return { 13.0, 13.0, 10.0 };
}
