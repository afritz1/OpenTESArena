#include "RenderInitSettings.h"

void RenderInitSettings::init(const Window *window, const std::string &dataFolderPath, int internalWidth, int internalHeight, int renderThreadsMode, DitheringMode ditheringMode)
{
    this->window = window;
    this->dataFolderPath = dataFolderPath;
    this->internalWidth = internalWidth;
    this->internalHeight = internalHeight;
    this->renderThreadsMode = renderThreadsMode;
    this->ditheringMode = ditheringMode;
}
