#include "LoadSaveUiModel.h"
#include "../Assets/ArenaSave.h"
#include "../Game/Game.h"
#include "../Math/Rect.h"
#include "../UI/TextAlignment.h"
#include "../Utilities/Platform.h"

#include "components/utilities/File.h"
#include "components/utilities/String.h"

void LoadSaveUiModel::Entry::init(std::string &&displayText)
{
	this->displayText = std::move(displayText);
}

std::string LoadSaveUiModel::getSavesPath(Game &game)
{
	const std::string &arenaSavesPath = game.getOptions().getMisc_ArenaSavesPath();
	const bool savesPathIsRelative = File::pathIsRelative(arenaSavesPath.c_str());
	const std::string path = (savesPathIsRelative ? Platform::getBasePath() : "") + arenaSavesPath;
	return String::addTrailingSlashIfMissing(path);
}

std::vector<LoadSaveUiModel::Entry> LoadSaveUiModel::getSaveEntries(Game &game)
{
	const std::string savesPath = LoadSaveUiModel::getSavesPath(game);
	const std::string fullSavesPath = savesPath + LoadSaveUiModel::ArenaSaveNamesFilename;
	if (!File::exists(fullSavesPath.c_str()))
	{
		DebugLogWarning("No " + LoadSaveUiModel::ArenaSaveNamesFilename + " found in \"" + savesPath + "\".");
		return std::vector<LoadSaveUiModel::Entry>();
	}

	const auto names = ArenaSave::loadNAMES(savesPath);
	std::vector<LoadSaveUiModel::Entry> entries;
	for (size_t i = 0; i < names->entries.size(); i++)
	{
		DebugAssertIndex(names->entries, i);
		const auto &nameEntry = names->entries[i];

		LoadSaveUiModel::Entry entry;
		entry.init(std::string(nameEntry.name.data()));
		entries.emplace_back(std::move(entry));
	}

	return entries;
}

std::optional<int> LoadSaveUiModel::getClickedIndex(const Int2 &originalPoint)
{
	constexpr int x = 2;
	constexpr int clickWidth = 316;
	constexpr int clickHeight = 13;
	constexpr int ySpacing = 1;

	int y = 2;
	for (int i = 0; i < LoadSaveUiModel::SlotCount; i++)
	{
		const Rect rect(x, y, clickWidth, clickHeight);
		if (rect.contains(originalPoint))
		{
			return i;
		}

		y += clickHeight + ySpacing;
	}

	return std::nullopt;
}
