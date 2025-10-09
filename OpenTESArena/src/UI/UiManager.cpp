#include "UiCommand.h"
#include "UiManager.h"

bool UiManager::init(const char *folderPath, TextureManager &textureManager, Renderer &renderer)
{
	// @todo eventually load UI asset txt files from folderPath

	// @todo preload some global things like cursor images

	return true;
}

void UiManager::shutdown(Renderer &renderer)
{
	this->renderElementsCache.clear();
}

void UiManager::populateCommandList(UiCommandList &commandList)
{
	commandList.addElements(this->renderElementsCache);
}
