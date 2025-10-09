#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <unordered_map>
#include <vector>

#include "../Rendering/Renderer.h"

class Game;
class TextureManager;

enum class UiScope;

struct UiCommandList;

using UiScopeCallback = void(*)(Game &game);
using UiScopeUpdateCallback = void(*)(double dt, Game &game);

class UiManager
{
private:
	std::unordered_map<UiScope, std::vector<UiScopeCallback>> beginScopeCallbackLists;
	std::unordered_map<UiScope, std::vector<UiScopeUpdateCallback>> updateScopeCallbackLists;
	std::unordered_map<UiScope, std::vector<UiScopeCallback>> endScopeCallbackLists;

	std::vector<UiScope> activeScopes;

	std::vector<RenderElement2D> renderElementsCache; // Updated every frame.

	void addBeginScopeCallback(UiScope scope, const UiScopeCallback &callback);
	void addUpdateScopeCallback(UiScope scope, const UiScopeUpdateCallback &callback);
	void addEndScopeCallback(UiScope scope, const UiScopeCallback &callback);
	void clearScopeCallbacks(UiScope scope);
public:
	bool init(const char *folderPath, TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	void beginScope(UiScope scope, Game &game);
	void endScope(UiScope scope, Game &game);

	void populateCommandList(UiCommandList &commandList);

	void update(double dt, Game &game);
};

#endif
