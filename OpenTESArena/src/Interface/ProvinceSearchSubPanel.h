#ifndef PROVINCE_SEARCH_SUB_PANEL_H
#define PROVINCE_SEARCH_SUB_PANEL_H

#include <string>
#include <vector>

#include "Panel.h"
#include "ProvinceMapUiModel.h"
#include "../Math/Vector2.h"
#include "../UI/ListBox.h"
#include "../UI/TextBox.h"
#include "../UI/Texture.h"

// The province search sub-panel lets the player enter a location name and travel to it
// as a convenience.

class ProvinceMapPanel;

class ProvinceSearchSubPanel : public Panel
{
private:
	Texture parchment;
	std::unique_ptr<TextBox> textTitleTextBox, textEntryTextBox;
	std::unique_ptr<ListBox> locationsListBox;
	Button<Game&, ProvinceSearchSubPanel&> textAcceptButton;
	Button<Game&, ProvinceSearchSubPanel&, int> listAcceptButton;
	Button<ListBox&> listUpButton, listDownButton;

	void handleTextEntryEvent(const SDL_Event &e);
	void handleListEvent(const SDL_Event &e);
	void renderTextEntry(Renderer &renderer);
	void renderList(Renderer &renderer);
public:
	// Public for UI controller.
	// - @todo: probably don't leave these as public forever
	ProvinceMapPanel *provinceMapPanel;
	std::vector<int> locationsListIndices;
	std::string locationName;
	ProvinceMapUiModel::SearchMode mode;
	int provinceID;

	ProvinceSearchSubPanel(Game &game);
	virtual ~ProvinceSearchSubPanel() = default;

	bool init(ProvinceMapPanel &provinceMapPanel, int provinceID);

	// Initializes the locations list box based on the locations list IDs.
	// - Public for UI controller
	void initLocationsListBox();

	virtual std::optional<Panel::CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
