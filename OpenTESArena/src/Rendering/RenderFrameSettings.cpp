#include "RenderFrameSettings.h"

RenderFrameSettings::RenderFrameSettings()
{
	this->ambientPercent = 0.0;
	this->paletteTextureID = -1;
	this->lightTableTextureID = -1;
	this->skyBgTextureID = -1;
	this->renderWidth = -1;
	this->renderHeight = -1;
	this->renderThreadsMode = -1;
	this->ditheringMode = -1;
}

void RenderFrameSettings::init(double ambientPercent, ObjectTextureID paletteTextureID, ObjectTextureID lightTableTextureID,
	ObjectTextureID skyBgTextureID, int renderWidth, int renderHeight, int renderThreadsMode, int ditheringMode)
{
	this->ambientPercent = ambientPercent;
	this->paletteTextureID = paletteTextureID;
	this->lightTableTextureID = lightTableTextureID;
	this->skyBgTextureID = skyBgTextureID;
	this->renderWidth = renderWidth;
	this->renderHeight = renderHeight;
	this->renderThreadsMode = renderThreadsMode;
	this->ditheringMode = ditheringMode;
}
