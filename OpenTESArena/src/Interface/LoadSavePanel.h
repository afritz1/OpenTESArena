#ifndef LOAD_SAVE_PANEL_H
#define LOAD_SAVE_PANEL_H

#include <array>

#include "Button.h"
#include "Panel.h"
#include "TextBox.h"

class Renderer;
class Surface;

class LoadSavePanel : public Panel
{
public:
	enum class Type { Load, Save };
private:
	static constexpr int SlotCount = 10;

	std::array<std::unique_ptr<TextBox>, 10> saveTextBoxes;
	Button<Game&, int> confirmButton;
	Button<Game&> backButton;
	LoadSavePanel::Type type;

	// Returns a non-negative integer if the point is contained in a save's click area.
	static int getClickedIndex(const Int2 &point);
public:
	LoadSavePanel(Game &game, LoadSavePanel::Type type);
	virtual ~LoadSavePanel() = default;

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
