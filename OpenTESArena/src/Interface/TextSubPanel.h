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
public:
	using OnClosedFunction = std::function<void(Game&)>;
private:
	TextBox textBox;
	Button<Game&> closeButton;
	ScopedUiTextureRef textureRef, cursorTextureRef;
	Int2 textureCenter;
public:
	TextSubPanel(Game &game);
	~TextSubPanel() override = default;

	bool init(const TextBox::InitInfo &textBoxInitInfo, const std::string_view text,
		const OnClosedFunction &onClosed, ScopedUiTextureRef &&textureRef, const Int2 &textureCenter);
};

#endif
