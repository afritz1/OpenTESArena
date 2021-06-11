#ifndef LOAD_SAVE_PANEL_H
#define LOAD_SAVE_PANEL_H

#include <memory>
#include <vector>

#include "Panel.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"

class Renderer;

class LoadSavePanel : public Panel
{
public:
	enum class Type { Load, Save };
private:
	std::vector<std::unique_ptr<TextBox>> saveTextBoxes;
	Button<Game&, int> confirmButton;
	Button<Game&> backButton;
	LoadSavePanel::Type type;
public:
	LoadSavePanel(Game &game);
	virtual ~LoadSavePanel() = default;

	bool init(LoadSavePanel::Type type);

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void render(Renderer &renderer) override;
};

#endif
