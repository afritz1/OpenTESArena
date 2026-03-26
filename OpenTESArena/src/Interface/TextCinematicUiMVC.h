#ifndef TEXT_CINEMATIC_UI_MVC_H
#define TEXT_CINEMATIC_UI_MVC_H

#include <string>
#include <vector>

#include "../Rendering/RenderTextureUtils.h"

#include "components/utilities/Buffer.h"

class Game;
class Renderer;
class TextureManager;

struct TextCinematicDefinition;

// Only used when speech files are available (in CD version).
class TextCinematicSpeechState
{
private:
	int templateDatKey, nextVoiceIndex;
public:
	TextCinematicSpeechState();

	void init(int templateDatKey);

	static bool isFirstVoice(int voiceIndex);
	static bool isBeginningOfNewPage(int voiceIndex);

	int getNextVoiceIndex() const;
	std::string getVoiceFilename(int voiceIndex) const;
	void incrementVoiceIndex();
	void resetVoiceIndex();
};

namespace TextCinematicUiModel
{
	bool shouldPlaySpeech(Game &game);

	std::string getSubtitleText(Game &game, const TextCinematicDefinition &textCinematicDef);

	// Gets the subtitle pages to be drawn individually.
	std::vector<std::string> getSubtitleTextPages(const std::string &text);
}

namespace TextCinematicUiView
{
	Buffer<UiTextureID> allocAnimationTextures(const std::string &animFilename, TextureManager &textureManager, Renderer &renderer);
}

#endif
