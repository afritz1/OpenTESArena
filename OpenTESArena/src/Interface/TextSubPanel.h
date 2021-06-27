#ifndef TEXT_SUB_PANEL_H
#define TEXT_SUB_PANEL_H

#include <functional>
#include <string>

#include "Panel.h"
#include "../Math/Vector2.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

// A simple sub-panel for displaying a text pop-up on-screen.

class TextSubPanel : public Panel
{
private:
	TextBox textBox;
	std::function<void(Game&)> onClosed;
	Texture texture;
	Int2 textureCenter;
public:
	TextSubPanel(Game &game);
	~TextSubPanel() override = default;

	bool init(const TextBox::InitInfo &textBoxInitInfo, const std::string_view &text,
		const std::function<void(Game&)> &onClosed, Texture &&texture, const Int2 &textureCenter);
	bool init(const TextBox::InitInfo &textBoxInitInfo, const std::string_view &text,
		const std::function<void(Game&)> &onClosed);

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
