#ifndef UI_LIBRARY_H
#define UI_LIBRARY_H

#include <vector>

#include "components/utilities/Singleton.h"

enum class UiContextType;

// @todo UiImageAsset? UiTextBoxAsset? ... they have the init info built in

struct UiImageAsset
{

};

struct UiTextBoxAsset
{

};

class UiLibrary : public Singleton<UiLibrary>
{
public:
	// @todo ui text assets etc
private:

public:
	bool init(const char *folderPath);

	// @todo get all ui assets of a UiContextType
	std::vector<char> getContextAssets(UiContextType contextType) const;
};

// @todo UiElementInstanceID UiManager::createAsset(const UiAsset&, UiContextType)

// @todo .txt format
// - context type at top
// - each ui component defined
// - component type, name, position, etc

#endif
