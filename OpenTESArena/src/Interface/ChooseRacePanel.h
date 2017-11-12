#ifndef CHOOSE_RACE_PANEL_H
#define CHOOSE_RACE_PANEL_H

#include <string>

#include "Button.h"
#include "Panel.h"
#include "../Entities/CharacterClass.h"
#include "../Rendering/Texture.h"

class Renderer;

enum class GenderName;

class ChooseRacePanel : public Panel
{
private:
	std::unique_ptr<Button<Game&, const CharacterClass&, const std::string&>> backToGenderButton;
	std::unique_ptr<Button<Game&, const CharacterClass&, const std::string&, 
		GenderName, int>> acceptButton;
	CharacterClass charClass;
	GenderName gender;
	std::string name;

	void drawProvinceTooltip(int provinceID, Renderer &renderer);
public:
	ChooseRacePanel(Game &game, const CharacterClass &charClass, 
		const std::string &name, GenderName gender);
	virtual ~ChooseRacePanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
