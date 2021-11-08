#include <algorithm>
#include <cmath>

#include "TextCinematicUiModel.h"
#include "../Game/Game.h"
#include "../Media/TextCinematicDefinition.h"

#include "components/dos/DOSUtils.h"
#include "components/utilities/String.h"

TextCinematicUiModel::SpeechState::SpeechState()
{
	this->templateDatKey = -1;
	this->nextVoiceIndex = -1;
}

void TextCinematicUiModel::SpeechState::init(int templateDatKey)
{
	this->templateDatKey = templateDatKey;
	this->nextVoiceIndex = 0;
}

bool TextCinematicUiModel::SpeechState::isFirstVoice(int voiceIndex)
{
	return voiceIndex == 0;
}

bool TextCinematicUiModel::SpeechState::isBeginningOfNewPage(int voiceIndex)
{
	return (voiceIndex % 2) == 0;
}

int TextCinematicUiModel::SpeechState::getNextVoiceIndex() const
{
	return this->nextVoiceIndex;
}

std::string TextCinematicUiModel::SpeechState::getVoiceFilename(int voiceIndex) const
{
	const int index = voiceIndex / 2;
	const char letter = SpeechState::isBeginningOfNewPage(voiceIndex) ? 'A' : 'B';

	DOSUtils::FilenameBuffer filename;
	std::snprintf(filename.data(), filename.size(), "%d_%02d%c.VOC",
		this->templateDatKey, index, letter);

	return "SPEECH/" + std::string(filename.data());
}

void TextCinematicUiModel::SpeechState::incrementVoiceIndex()
{
	this->nextVoiceIndex++;
}

void TextCinematicUiModel::SpeechState::resetVoiceIndex()
{
	this->nextVoiceIndex = 0;
}

bool TextCinematicUiModel::shouldPlaySpeech(Game &game)
{
	const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
	const auto &exeData = binaryAssetLibrary.getExeData();
	return !exeData.isFloppyVersion();
}

std::string TextCinematicUiModel::getSubtitleText(Game &game, const TextCinematicDefinition &textCinematicDef)
{
	const auto &textAssetLibrary = game.getTextAssetLibrary();
	const auto &templateDat = textAssetLibrary.getTemplateDat();
	const auto &templateDatEntry = templateDat.getEntry(textCinematicDef.getTemplateDatKey());
	std::string cinematicText = templateDatEntry.values.front();
	cinematicText.push_back('\n');

	// Replace substitution tokens. The original game wraps text onto the next screen if the
	// player's name is too long, which may push the text for every subsequent screen forward
	// by a little bit.
	const auto &player = game.getGameState().getPlayer();
	const std::string playerFirstName = player.getFirstName();
	cinematicText = String::replace(cinematicText, "%pcf", playerFirstName);

	// Re-distribute newlines.
	const std::string newText = String::distributeNewlines(cinematicText, 60);

	// Some more formatting should be done in the future so the text wraps nicer. That is,
	// replace all new lines with spaces and redistribute new lines given some max line
	// length value.

	return newText;
}

std::vector<std::string> TextCinematicUiModel::getSubtitleTextPages(const std::string &text)
{
	const int textLineCount = static_cast<int>(std::count(text.begin(), text.end(), '\n'));
	const int textPagesToMake = static_cast<int>(std::ceil(textLineCount / 3)) + 1;
	const std::vector<std::string> textLines = String::split(text, '\n');

	// Group up to three text lines per text box.
	std::vector<std::string> textPages;
	for (int i = 0; i < textPagesToMake; i++)
	{
		std::string textPageText;
		int linesToUse = std::min(textLineCount - (i * 3), 3);
		for (int j = 0; j < linesToUse; j++)
		{
			const int textLineIndex = j + (i * 3);
			DebugAssertIndex(textLines, textLineIndex);
			const std::string &textLine = textLines[textLineIndex];

			if (textPageText.length() > 0)
			{
				textPageText.append("\n");
			}

			textPageText.append(textLine);
		}

		if (textPageText.size() > 0)
		{
			textPages.emplace_back(std::move(textPageText));
		}
	}

	return textPages;
}
