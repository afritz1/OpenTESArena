#include "LoadSaveUiModel.h"
#include "../Assets/ArenaSave.h"
#include "../Game/Game.h"
#include "../Math/Rect.h"
#include "../UI/TextAlignment.h"
#include "../Utilities/Platform.h"

#include "components/utilities/File.h"
#include "components/utilities/Path.h"
#include "components/utilities/String.h"

void LoadSaveUiEntry::init(const std::string &text)
{
	this->text = text;
}

std::string LoadSaveUiModel::getSavesPath(Game &game)
{
	const std::string &arenaSavesPath = game.options.getMisc_ArenaSavesPath();
	const bool savesPathIsRelative = Path::isRelative(arenaSavesPath.c_str());
	const std::string path = (savesPathIsRelative ? Platform::getBasePath() : "") + arenaSavesPath;
	return String::addTrailingSlashIfMissing(path);
}

std::vector<LoadSaveUiEntry> LoadSaveUiModel::getSaveEntries(Game &game)
{
	const std::string savesPath = LoadSaveUiModel::getSavesPath(game);
	const std::string fullSavesPath = savesPath + LoadSaveUiModel::ArenaSaveNamesFilename;
	if (!File::exists(fullSavesPath.c_str()))
	{
		DebugLogWarningFormat("No %s found in \"%s\".", LoadSaveUiModel::ArenaSaveNamesFilename.c_str(), savesPath.c_str());
		return std::vector<LoadSaveUiEntry>();
	}

	const auto names = ArenaSave::loadNAMES(savesPath);
	std::vector<LoadSaveUiEntry> entries;
	for (size_t i = 0; i < names->entries.size(); i++)
	{
		DebugAssertIndex(names->entries, i);
		const ArenaTypes::Names::Entry &nameEntry = names->entries[i];

		LoadSaveUiEntry entry;
		entry.init(nameEntry.name.data());
		entries.emplace_back(std::move(entry));
	}

	return entries;
}

Rect LoadSaveUiModel::getSlotRect(int index)
{
	constexpr int clickWidth = 316;
	constexpr int clickHeight = 13;
	constexpr int ySpacing = 1;

	return Rect(
		2,
		2 + ((clickHeight + ySpacing) * index),
		clickWidth,
		clickHeight);
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
