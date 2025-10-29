#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "UiElement.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/KeyValuePool.h"

class Game;
class TextureManager;

enum class UiContextType;

struct UiCommandList;

using UiContextCallback = void(*)(Game &game);
using UiContextUpdateCallback = void(*)(double dt, Game &game);

class UiManager
{
private:
	KeyValuePool<UiTransformInstanceID, UiTransform> transforms;
	KeyValuePool<UiElementInstanceID, UiElement> elements;
	KeyValuePool<UiImageInstanceID, UiImage> images;
	KeyValuePool<UiTextBoxInstanceID, UiTextBox> textBoxes;
	KeyValuePool<UiButtonInstanceID, UiButton> buttons;

	std::unordered_map<UiContextType, std::vector<UiContextCallback>> beginContextCallbackLists;
	std::unordered_map<UiContextType, std::vector<UiContextUpdateCallback>> updateContextCallbackLists;
	std::unordered_map<UiContextType, std::vector<UiContextCallback>> endContextCallbackLists;
	std::optional<UiContextType> activeContextType;

	std::vector<RenderElement2D> renderElementsCache; // To be drawn. Updated every frame.
public:
	bool init(const char *folderPath, TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	void setElementActive(UiElementInstanceID elementInstID, bool active);

	void setTransformPosition(UiElementInstanceID elementInstID, Int2 position);
	void setTransformSize(UiElementInstanceID elementInstID, Int2 size);

	UiElementInstanceID createImage(const UiElementInitInfo &initInfo, UiTextureID textureID);
	void setImageTexture(UiElementInstanceID elementInstID, UiTextureID textureID);
	void freeImage(UiElementInstanceID elementInstID);

	UiElementInstanceID createTextBox(const UiElementInitInfo &initInfo); // @todo provide everything for creating the texture (dimensions, font) except text
	void setTextBoxText(UiElementInstanceID elementInstID, const char *str);
	void freeTextBox(UiElementInstanceID elementInstID);

	UiElementInstanceID createButton(const UiElementInitInfo &initInfo); // @todo provide button size + callback
	void freeButton(UiElementInstanceID elementInstID);

	void addBeginContextCallback(UiContextType contextType, const UiContextCallback &callback);
	void addUpdateContextCallback(UiContextType contextType, const UiContextUpdateCallback &callback);
	void addEndContextCallback(UiContextType contextType, const UiContextCallback &callback);
	void clearContextCallbacks(UiContextType contextType);

	void beginContext(UiContextType contextType, Game &game);
	void endContext(UiContextType contextType, Game &game);
	bool isContextActive(UiContextType contextType) const;

	void populateCommandList(UiCommandList &commandList);

	void update(double dt, Game &game);
};

#endif
