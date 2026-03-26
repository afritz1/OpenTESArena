#include "CinematicUiState.h"
#include "ImageSequenceUiState.h"
#include "ImageUiState.h"
#include "IntroUiMVC.h"
#include "MainMenuUiState.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/TextureAsset.h"
#include "../Game/Game.h"

std::string IntroUiModel::prepareStartupContext(Game &game)
{
	if (!game.options.getMisc_ShowIntro())
	{
		return MainMenuUI::ContextName;
	}

	const ExeData &exeData = BinaryAssetLibrary::getInstance().getExeData();
	const bool isFloppyVersion = exeData.isFloppyVersion;
	if (isFloppyVersion)
	{
		ImageUiInitInfo &imageInitInfo = ImageUI::state.initInfo;
		imageInitInfo.init(ArenaTextureName::IntroTitle, ArenaTextureName::IntroTitle, IntroUiView::IntroTitleSeconds, [&game]() { IntroUiController::onIntroTitleFinished(game); });
		return ImageUI::ContextName;
	}
	else
	{
		const std::string paletteFilename = IntroUiView::getIntroBookPaletteFilename();
		const std::string sequenceFilename = IntroUiView::getIntroBookSequenceFilename();

		TextureManager &textureManager = game.textureManager;
		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceFilename.c_str());
		if (!metadataID.has_value())
		{
			DebugLogErrorFormat("Couldn't get texture file metadata for start-up cinematic \"%s\".", sequenceFilename.c_str());
			return nullptr;
		}

		const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);

		CinematicUiInitInfo &cinematicInitInfo = CinematicUI::state.initInfo;
		cinematicInitInfo.init(paletteFilename, sequenceFilename, metadata.getSecondsPerFrame(), [&game]() { IntroUiController::onIntroBookFinished(game); });
		return CinematicUI::ContextName;
	}
}

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

void IntroUiController::onIntroBookFinished(Game &game)
{
	const TextureAsset paletteTextureAsset = IntroUiView::getIntroTitlePaletteTextureAsset();
	const TextureAsset textureAsset = IntroUiView::getIntroTitleTextureAsset();

	ImageUiInitInfo &imageInitInfo = ImageUI::state.initInfo;
	imageInitInfo.init(paletteTextureAsset.filename, textureAsset.filename, IntroUiView::IntroTitleSeconds, [&game]() { IntroUiController::onIntroTitleFinished(game); });
	game.setNextContext(ImageUI::ContextName);
}

void IntroUiController::onIntroTitleFinished(Game &game)
{
	const TextureAsset paletteTextureAsset = IntroUiView::getIntroQuotePaletteTextureAsset();
	const TextureAsset textureAsset = IntroUiView::getIntroQuoteTextureAsset();

	ImageUiInitInfo &imageInitInfo = ImageUI::state.initInfo;
	imageInitInfo.init(paletteTextureAsset.filename, textureAsset.filename, IntroUiView::IntroQuoteSeconds, [&game]() { IntroUiController::onIntroQuoteFinished(game); });
	game.setNextContext(ImageUI::ContextName);
}

void IntroUiController::onIntroQuoteFinished(Game &game)
{
	const std::string paletteFilename = IntroUiView::getOpeningScrollPaletteFilename();
	const std::string sequenceFilename = IntroUiView::getOpeningScrollSequenceFilename();

	TextureManager &textureManager = game.textureManager;
	const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(sequenceFilename.c_str());
	if (!metadataID.has_value())
	{
		DebugLogErrorFormat("Couldn't get texture file metadata for opening scroll animation \"%s\".", sequenceFilename.c_str());
		return;
	}

	const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);

	CinematicUiInitInfo &cinematicInitInfo = CinematicUI::state.initInfo;
	cinematicInitInfo.init(paletteFilename, sequenceFilename, metadata.getSecondsPerFrame(), [&game]() { IntroUiController::onOpeningScrollFinished(game); });
	game.setNextContext(CinematicUI::ContextName);
}

void IntroUiController::onOpeningScrollFinished(Game &game)
{
	ImageSequenceUiInitInfo &imageSequenceInitInfo = ImageSequenceUI::state.initInfo;
	imageSequenceInitInfo.init(IntroUiView::getIntroStoryPaletteNames(), IntroUiView::getIntroStoryTextureNames(), IntroUiView::getIntroStoryImageDurations(), [&game]() { IntroUiController::onIntroStoryFinished(game); });
	game.setNextContext(ImageSequenceUI::ContextName);
}

void IntroUiController::onIntroStoryFinished(Game &game)
{
	game.setNextContext(MainMenuUI::ContextName);
}
