#include "RenderInitSettings.h"

void RenderInitSettings::init(int width, int height, int renderThreadsMode, DitheringMode ditheringMode)
{
    this->width = width;
    this->height = height;
    this->renderThreadsMode = renderThreadsMode;
    this->ditheringMode = ditheringMode;
}
