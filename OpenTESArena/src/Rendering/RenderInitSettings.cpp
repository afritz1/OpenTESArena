#include "RenderInitSettings.h"

RenderContextSettings::RenderContextSettings()
{
	this->window = nullptr;
	this->enableValidationLayers = false;
}

void RenderContextSettings::init(const Window *window, bool enableValidationLayers)
{
	this->window = window;
	this->enableValidationLayers = enableValidationLayers;
}

RenderInitSettings::RenderInitSettings()
{
    this->window = nullptr;
    this->internalWidth = 0;
    this->internalHeight = 0;
    this->renderThreadsMode = 0;
    this->ditheringMode = static_cast<DitheringMode>(-1);
}

void RenderInitSettings::init(const Window *window, const std::string &dataFolderPath, int internalWidth, int internalHeight, int renderThreadsMode,
    DitheringMode ditheringMode)
{
    this->window = window;
    this->dataFolderPath = dataFolderPath;
    this->internalWidth = internalWidth;
    this->internalHeight = internalHeight;
    this->renderThreadsMode = renderThreadsMode;
    this->ditheringMode = ditheringMode;
}
