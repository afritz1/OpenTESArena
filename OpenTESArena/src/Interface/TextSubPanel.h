#ifndef TEXT_SUB_PANEL_H
#define TEXT_SUB_PANEL_H

#include <functional>
#include <string>

#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Rendering/Texture.h"

// A simple sub-panel for displaying a text pop-up on-screen.

class RichTextString;
class TextBox;

class TextSubPanel : public Panel
{
private:
	std::unique_ptr<TextBox> textBox;
	std::function<void(Game&)> endingAction;
	Texture texture;
	Int2 textureCenter;
public:
	TextSubPanel(Game &game, const Int2 &textCenter, const RichTextString &richText,
		const std::function<void(Game&)> &endingAction, Texture &&texture,
		const Int2 &textureCenter);
	TextSubPanel(Game &game, const Int2 &textCenter, const RichTextString &richText,
		const std::function<void(Game&)> &endingAction);
	virtual ~TextSubPanel() = default;

	virtual std::pair<const Texture*, CursorAlignment> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
