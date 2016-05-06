#ifndef CHARACTER_PANEL_H
#define CHARACTER_PANEL_H

#include "Panel.h"

// This is the character portrait panel that shows the player's attributes and
// derived stats.

class Button;
class CharacterClass;
class TextBox;

enum class CharacterGenderName;
enum class CharacterRaceName;

class CharacterPanel : public Panel
{
private:
	std::unique_ptr<Button> backToGameButton;
protected:
	virtual void handleEvents(bool &running) override;
	virtual void handleMouse(double dt) override;
	virtual void handleKeyboard(double dt) override;
public:
	CharacterPanel(GameState *gameState);
	virtual ~CharacterPanel();

	virtual void tick(double dt, bool &running) override;
	virtual void render(SDL_Surface *dst, const SDL_Rect *letterbox) override;
};

#endif
