#ifndef PROVINCE_SEARCH_SUB_PANEL_H
#define PROVINCE_SEARCH_SUB_PANEL_H

#include <string>
#include <vector>

#include "ListBox.h"
#include "Panel.h"
#include "TextBox.h"
#include "../Math/Vector2.h"
#include "../Rendering/Texture.h"

// The province search sub-panel lets the player enter a location name and travel to it
// as a convenience.

class CityDataFile;
class ProvinceMapPanel;

class ProvinceSearchSubPanel : public Panel
{
private:
	enum class Mode { TextEntry, List };

	static const int MAX_NAME_LENGTH;
	static const Int2 DEFAULT_TEXT_CURSOR_POSITION;

	Texture parchment;
	std::unique_ptr<TextBox> textTitleTextBox, textEntryTextBox;
	std::unique_ptr<ListBox> locationsListBox;
	Button<Game&, ProvinceSearchSubPanel&> textAcceptButton;
	Button<Game&, ProvinceSearchSubPanel&, int> listAcceptButton;
	Button<ProvinceSearchSubPanel&> listUpButton, listDownButton;
	ProvinceMapPanel &provinceMapPanel;
	std::vector<int> locationsListIDs;
	std::string locationName;
	Mode mode;
	int provinceID;

	// Returns a list of all visible location IDs in the given province that have a match with
	// the given location name. Technically, this should only return up to one ID, but returning
	// a list allows functionality for approximate matches. The exact location ID points into
	// the vector if there is an exact match, or null otherwise.
	static std::vector<int> getMatchingLocations(const std::string &locationName, int provinceID,
		const CityDataFile &cityData, const int **exactLocationID);

	// Gets the .IMG filename of the background image.
	std::string getBackgroundFilename() const;

	// Initializes the locations list box based on the locations list IDs.
	void initLocationsListBox();

	void handleTextEntryEvent(const SDL_Event &e);
	void handleListEvent(const SDL_Event &e);
	void renderTextEntry(Renderer &renderer);
	void renderList(Renderer &renderer);
public:
	ProvinceSearchSubPanel(Game &game, ProvinceMapPanel &provinceMapPanel, int provinceID);
	virtual ~ProvinceSearchSubPanel() = default;

	virtual Panel::CursorData getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
