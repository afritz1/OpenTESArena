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

class ProvinceMapPanel;

// The province search sub-panel lets the player enter a location name and travel to it
// as a convenience.
class ProvinceSearchSubPanel : public Panel
{
private:
	TextBox textTitleTextBox, textEntryTextBox;
	ListBox locationsListBox;
	Button<ListBox&> listUpButton, listDownButton;
	ScopedUiTextureRef parchmentTextureRef, listBackgroundTextureRef, cursorTextureRef;
public:
	// Public for UI controller.
	// - @todo: probably don't leave these as public forever
	ProvinceMapPanel *provinceMapPanel;
	std::vector<int> locationsListIndices;
	std::string locationName;
	ProvinceSearchUiModel::Mode mode;
	int provinceID;

	ProvinceSearchSubPanel(Game &game);
	~ProvinceSearchSubPanel() override = default;

	bool init(ProvinceMapPanel &provinceMapPanel, int provinceID);

	// Initializes the locations list screen based on the locations list IDs.
	// - Public for UI controller
	void initLocationsList();

	void tick(double dt) override;
};

#endif
