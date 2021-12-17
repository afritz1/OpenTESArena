#include "RenderInitSettings.h"

void RenderInitSettings::init(int width, int height, int renderThreadsMode)
{
    this->width = width;
    this->height = height;
    this->renderThreadsMode = renderThreadsMode;
}
