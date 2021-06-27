#ifndef MESSAGE_BOX_SUB_PANEL_H
#define MESSAGE_BOX_SUB_PANEL_H

#include <functional>
#include <vector>

#include "Panel.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

// A sub-panel intended for displaying text with some buttons.
class MessageBoxSubPanel : public Panel
{
public:
	// The title or description of the message box.
	struct Title
	{
		TextBox textBox;
		Texture texture;
		int textureX, textureY;
	};

	// An element of the message box (i.e., a button with text).
	struct Element
	{
		TextBox textBox;
		Texture texture;
		std::function<void(Game&)> function;
		int textureX, textureY;
	};
private:
	MessageBoxSubPanel::Title title;
	std::vector<MessageBoxSubPanel::Element> elements;
	std::function<void(Game&)> cancelFunction; // Called when cancelling the message box.
public:
	MessageBoxSubPanel(Game &game);
	~MessageBoxSubPanel() override = default;

	bool init(MessageBoxSubPanel::Title &&title, std::vector<MessageBoxSubPanel::Element> &&elements, const std::function<void(Game&)> &cancelFunction);
	bool init(MessageBoxSubPanel::Title &&title, std::vector<MessageBoxSubPanel::Element> &&elements);

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
