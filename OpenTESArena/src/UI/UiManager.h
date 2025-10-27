#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <unordered_map>
#include <vector>

#include "UiElement.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/KeyValuePool.h"

class Game;
class TextureManager;

enum class UiScope;

struct UiCommandList;

using UiScopeCallback = void(*)(Game &game);
using UiScopeUpdateCallback = void(*)(double dt, Game &game);

class UiManager
{
private:
	KeyValuePool<UiTransformInstanceID, UiTransform> transforms;
	KeyValuePool<UiElementInstanceID, UiElement> elements;
	KeyValuePool<UiImageInstanceID, UiImage> images;
	KeyValuePool<UiTextBoxInstanceID, UiTextBox> textBoxes;
	KeyValuePool<UiButtonInstanceID, UiButton> buttons;

	std::unordered_map<UiScope, std::vector<UiScopeCallback>> beginScopeCallbackLists;
	std::unordered_map<UiScope, std::vector<UiScopeUpdateCallback>> updateScopeCallbackLists;
	std::unordered_map<UiScope, std::vector<UiScopeCallback>> endScopeCallbackLists;
	std::vector<UiScope> activeScopes;

	std::vector<RenderElement2D> renderElementsCache; // To be drawn. Updated every frame.

	void addBeginScopeCallback(UiScope scope, const UiScopeCallback &callback);
	void addUpdateScopeCallback(UiScope scope, const UiScopeUpdateCallback &callback);
	void addEndScopeCallback(UiScope scope, const UiScopeCallback &callback);
	void clearScopeCallbacks(UiScope scope);
public:
	bool init(const char *folderPath, TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	void setElementActive(UiElementInstanceID elementInstID, bool active);

	UiElementInstanceID createImage(UiScope scope, UiTextureID textureID);
	void setImageTexture(UiElementInstanceID elementInstID, UiTextureID textureID);
	void freeImage(UiElementInstanceID elementInstID);

	UiElementInstanceID createTextBox(UiScope scope); // @todo provide everything for creating the texture (dimensions, font) except text
	void setTextBoxText(UiElementInstanceID elementInstID, const char *str);
	void freeTextBox(UiElementInstanceID elementInstID);

	UiElementInstanceID createButton(UiScope scope); // @todo provide button size + callback
	void freeButton(UiElementInstanceID elementInstID);

	void beginScope(UiScope scope, Game &game);
	void endScope(UiScope scope, Game &game);
	bool isScopeActive(UiScope scope) const;

	void populateCommandList(UiCommandList &commandList);

	void update(double dt, Game &game);
};

#endif
