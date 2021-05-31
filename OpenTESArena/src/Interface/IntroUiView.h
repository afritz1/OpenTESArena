#ifndef INTRO_UI_VIEW_H
#define INTRO_UI_VIEW_H

#include <string>
#include <vector>

struct TextureAssetReference;

namespace IntroUiView
{
	constexpr double IntroBookFramesPerSecond = 7.0;
	std::string getIntroBookSequenceFilename();
	std::string getIntroBookPaletteFilename();

	constexpr double IntroTitleSeconds = 5.0;
	TextureAssetReference getIntroTitleTextureAssetReference();
	TextureAssetReference getIntroTitlePaletteTextureAssetReference();

	constexpr double IntroQuoteSeconds = 5.0;
	TextureAssetReference getIntroQuoteTextureAssetReference();
	TextureAssetReference getIntroQuotePaletteTextureAssetReference();

	constexpr double OpeningScrollFramesPerSecond = 24.0;
	std::string getOpeningScrollSequenceFilename();
	std::string getOpeningScrollPaletteFilename();

	std::vector<std::string> getIntroStoryTextureNames();
	std::vector<std::string> getIntroStoryPaletteNames();
	std::vector<double> getIntroStoryImageDurations();
}

#endif
