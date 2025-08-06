#include "RenderInitSettings.h"

void RenderInitSettings::init(SDL_Window *window, const std::string &dataFolderPath, int width, int height, int renderThreadsMode, DitheringMode ditheringMode)
{
    this->window = window;
    this->dataFolderPath = dataFolderPath;
    this->width = width;
    this->height = height;
    this->renderThreadsMode = renderThreadsMode;
    this->ditheringMode = ditheringMode;
}
