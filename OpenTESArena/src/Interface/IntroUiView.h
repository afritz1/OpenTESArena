#ifndef INTRO_UI_VIEW_H
#define INTRO_UI_VIEW_H

#include <string>

#include "components/utilities/Buffer.h"

struct TextureAsset;

namespace IntroUiView
{
	std::string getIntroBookSequenceFilename();
	std::string getIntroBookPaletteFilename();

	constexpr double IntroTitleSeconds = 5.0;
	TextureAsset getIntroTitleTextureAsset();
	TextureAsset getIntroTitlePaletteTextureAsset();

	constexpr double IntroQuoteSeconds = 5.0;
	TextureAsset getIntroQuoteTextureAsset();
	TextureAsset getIntroQuotePaletteTextureAsset();

	std::string getOpeningScrollSequenceFilename();
	std::string getOpeningScrollPaletteFilename();

	Buffer<std::string> getIntroStoryTextureNames();
	Buffer<std::string> getIntroStoryPaletteNames();
	Buffer<double> getIntroStoryImageDurations();
}

#endif
