#ifndef INTRO_UI_MVC_H
#define INTRO_UI_MVC_H

#include <string>

#include "components/utilities/Buffer.h"

class Game;

struct TextureAsset;

namespace IntroUiModel
{
	// Decides which UI context to startup with and prepares relevant global UI values.
	std::string prepareStartupContext(Game &game);
}

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

namespace IntroUiController
{
	void onIntroBookFinished(Game &game);
	void onIntroTitleFinished(Game &game);
	void onIntroQuoteFinished(Game &game);
	void onOpeningScrollFinished(Game &game);
	void onIntroStoryFinished(Game &game);
}

#endif
