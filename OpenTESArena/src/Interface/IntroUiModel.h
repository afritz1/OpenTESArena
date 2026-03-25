#ifndef INTRO_UI_MODEL_H
#define INTRO_UI_MODEL_H

#include <string>

class Game;

namespace IntroUiModel
{
	// Decides which UI context to startup with and prepares relevant global UI values.
	std::string prepareStartupContext(Game &game);
}

#endif
