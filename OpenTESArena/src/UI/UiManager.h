#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <vector>

#include "../Rendering/Renderer.h"

class TextureManager;

struct UiCommandList;

class UiManager
{
private:
	std::vector<RenderElement2D> renderElementsCache; // Updated every frame.
public:
	bool init(const char *folderPath, TextureManager &textureManager, Renderer &renderer);
	void shutdown(Renderer &renderer);

	void populateCommandList(UiCommandList &commandList);
};

#endif
