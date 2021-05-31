#ifndef INTRO_UI_MODEL_H
#define INTRO_UI_MODEL_H

#include <memory>

class Game;
class Panel;

namespace IntroUiModel
{
	// Initial panel assigned at engine start.
	std::unique_ptr<Panel> makeStartupPanel(Game &game);
}

#endif
