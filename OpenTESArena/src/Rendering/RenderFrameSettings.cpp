#include "RenderFrameSettings.h"

void RenderFrameSettings::init(double ambientPercent, ObjectTextureID paletteTextureID, ObjectTextureID lightTableTextureID,
	int renderWidth, int renderHeight, int renderThreadsMode)
{
	this->ambientPercent = ambientPercent;
	this->paletteTextureID = paletteTextureID;
	this->lightTableTextureID = lightTableTextureID;
	this->renderWidth = renderWidth;
	this->renderHeight = renderHeight;
	this->renderThreadsMode = renderThreadsMode;
}
