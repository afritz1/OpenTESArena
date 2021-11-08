#ifndef TEXT_CINEMATIC_UI_MODEL_H
#define TEXT_CINEMATIC_UI_MODEL_H

#include <string>
#include <vector>

class Game;
class TextCinematicDefinition;

namespace TextCinematicUiModel
{
	// Only used when speech files are available (in CD version).
	class SpeechState
	{
	private:
		int templateDatKey, nextVoiceIndex;
	public:
		SpeechState();

		void init(int templateDatKey);

		static bool isFirstVoice(int voiceIndex);
		static bool isBeginningOfNewPage(int voiceIndex);

		int getNextVoiceIndex() const;
		std::string getVoiceFilename(int voiceIndex) const;
		void incrementVoiceIndex();
		void resetVoiceIndex();
	};

	bool shouldPlaySpeech(Game &game);

	std::string getSubtitleText(Game &game, const TextCinematicDefinition &textCinematicDef);

	// Gets the subtitle pages to be drawn individually.
	std::vector<std::string> getSubtitleTextPages(const std::string &text);
}

#endif
