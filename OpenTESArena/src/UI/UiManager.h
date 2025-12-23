#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "TextRenderUtils.h"
#include "UiButton.h"
#include "UiElement.h"
#include "UiImage.h"
#include "UiTextBox.h"
#include "UiTransform.h"
#include "../Assets/TextureAsset.h"
#include "../Rendering/Renderer.h"

#include "components/utilities/KeyValuePool.h"

class Game;
class InputManager;
class TextureManager;

enum class UiContextType;

struct UiCommandList;
struct UiContextDefinition;
struct UiContextState;

using UiContextBeginCallback = void(*)(Game &game);
using UiContextEndCallback = void(*)();
using UiContextUpdateCallback = void(*)(double dt);

struct LoadedUiTexture
{
	// @todo use context type as well so it's not taking up space forever
	TextureAsset textureAsset;
	TextureAsset paletteAsset;
	UiTextureID textureID;

	LoadedUiTexture();
};

struct GeneratedUiTexture
{
	// @todo use context type as well so it's not taking up space forever
	UiTexturePatternType patternType;
	int width;
	int height;
	UiTextureID textureID;

	GeneratedUiTexture();
};

class UiManager
{
private:
	KeyValuePool<UiTransformInstanceID, UiTransform> transforms;
	KeyValuePool<UiElementInstanceID, UiElement> elements;
	KeyValuePool<UiImageInstanceID, UiImage> images;
	KeyValuePool<UiTextBoxInstanceID, UiTextBox> textBoxes;
	KeyValuePool<UiButtonInstanceID, UiButton> buttons;

	std::vector<LoadedUiTexture> loadedTextures;
	std::vector<GeneratedUiTexture> generatedTextures;

	std::unordered_map<UiContextType, std::vector<UiContextBeginCallback>> beginContextCallbackLists;
	std::unordered_map<UiContextType, std::vector<UiContextUpdateCallback>> updateContextCallbackLists;
	std::unordered_map<UiContextType, std::vector<UiContextEndCallback>> endContextCallbackLists;
	std::optional<UiContextType> activeContextType;

	std::vector<RenderElement2D> renderElementsCache; // To be drawn. Updated every frame.

	UiTextureID getOrAddTexture(const TextureAsset &textureAsset, const TextureAsset &paletteAsset, TextureManager &textureManager, Renderer &renderer);
	UiTextureID getOrAddTexture(UiTexturePatternType patternType, int width, int height, TextureManager &textureManager, Renderer &renderer);
public:
	bool init(const char *folderPath, TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	UiElementInstanceID getElementByName(const char *name) const;
	void setElementActive(UiElementInstanceID elementInstID, bool active);

	Rect getTransformGlobalRect(UiElementInstanceID elementInstID) const; // Includes local-to-parent transform.
	void setTransformPosition(UiElementInstanceID elementInstID, Int2 position);
	void setTransformSize(UiElementInstanceID elementInstID, Int2 size);
	void setTransformPivot(UiElementInstanceID elementInstID, UiPivotType pivotType);

	const UiButtonCallback &getButtonCallback(UiElementInstanceID elementInstID) const;
	bool isMouseButtonValidForButton(MouseButtonType mouseButtonType, UiElementInstanceID elementInstID) const;
	
	std::vector<UiElementInstanceID> getActiveButtonInstIDs() const;

	UiElementInstanceID createImage(const UiElementInitInfo &initInfo, UiTextureID textureID, UiContextType contextType, UiContextState &contextState);
	void setImageTexture(UiElementInstanceID elementInstID, UiTextureID textureID);
	void freeImage(UiElementInstanceID elementInstID);

	UiElementInstanceID createTextBox(const UiElementInitInfo &initInfo, const UiTextBoxInitInfo &textBoxInitInfo, UiContextType contextType, UiContextState &contextState, Renderer &renderer);
	void setTextBoxText(UiElementInstanceID elementInstID, const char *str);
	void freeTextBox(UiElementInstanceID elementInstID, Renderer &renderer);

	UiElementInstanceID createButton(const UiElementInitInfo &initInfo, const UiButtonInitInfo &buttonInitInfo, UiContextType contextType, UiContextState &contextState);
	void freeButton(UiElementInstanceID elementInstID);

	void addInputActionListener(const char *actionName, const InputActionCallback &callback, InputManager &inputManager, UiContextState &contextState);

	void addBeginContextCallback(UiContextType contextType, const UiContextBeginCallback &callback);
	void addEndContextCallback(UiContextType contextType, const UiContextEndCallback &callback);
	void addUpdateContextCallback(UiContextType contextType, const UiContextUpdateCallback &callback);
	void clearContextCallbacks(UiContextType contextType);

	void beginContext(UiContextType contextType, Game &game);
	void endContext(UiContextType contextType, Game &game);
	bool isContextActive(UiContextType contextType) const;
	void createContext(const UiContextDefinition &contextDef, UiContextState &contextState, InputManager &inputManager,
		TextureManager &textureManager, Renderer &renderer);

	void populateCommandList(UiCommandList &commandList);

	void update(double dt, Game &game);
};

#endif
