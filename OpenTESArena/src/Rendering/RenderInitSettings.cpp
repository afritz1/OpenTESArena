#include "RenderInitSettings.h"

void RenderInitSettings::init(int width, int height, int renderThreadsMode)
{
    this->width = width;
    this->height = height;
    this->renderThreadsMode = renderThreadsMode;
}

int RenderInitSettings::getWidth() const
{
    return width;
}

int RenderInitSettings::getHeight() const
{
    return height;
}

int RenderInitSettings::getRenderThreadsMode() const
{
    return renderThreadsMode;
}
