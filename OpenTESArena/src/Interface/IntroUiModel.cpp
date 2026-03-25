#include "CinematicUiState.h"
#include "ImageUiState.h"
#include "IntroUiController.h"
#include "IntroUiModel.h"
#include "IntroUiView.h"
#include "MainMenuUiState.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/BinaryAssetLibrary.h"
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
