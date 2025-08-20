#include "RenderInitSettings.h"

RenderInitSettings::RenderInitSettings()
{
    this->window = nullptr;
    this->internalWidth = 0;
    this->internalHeight = 0;
    this->renderThreadsMode = 0;
    this->ditheringMode = static_cast<DitheringMode>(-1);
    this->enableValidationLayers = false;
}

void RenderInitSettings::init(const Window *window, const std::string &dataFolderPath, int internalWidth, int internalHeight, int renderThreadsMode,
    DitheringMode ditheringMode, bool enableValidationLayers)
{
    this->window = window;
    this->dataFolderPath = dataFolderPath;
    this->internalWidth = internalWidth;
    this->internalHeight = internalHeight;
    this->renderThreadsMode = renderThreadsMode;
    this->ditheringMode = ditheringMode;
    this->enableValidationLayers = enableValidationLayers;
}
