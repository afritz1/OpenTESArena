#ifndef MESSAGE_BOX_SUB_PANEL_H
#define MESSAGE_BOX_SUB_PANEL_H

#include <functional>
#include <string>
#include <vector>

#include "Panel.h"
#include "../Rendering/Texture.h"

class TextBox;

// A sub-panel intended for displaying text with some buttons.
class MessageBoxSubPanel : public Panel
{
private:
	std::unique_ptr<TextBox> textBox;
	Texture textBoxTexture, buttonTexture;
	std::vector<std::function<void(Game&)>> functions;
public:
	MessageBoxSubPanel(Game &game, std::unique_ptr<TextBox> textBox,
		Texture &&textBoxTexture, Texture &&buttonTexture,
		const std::vector<std::function<void(Game&)>> &functions);
	virtual ~MessageBoxSubPanel();

	virtual std::pair<SDL_Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
