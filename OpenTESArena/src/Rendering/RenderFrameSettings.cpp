#include "RenderCamera.h"
#include "RenderFrameSettings.h"

void RenderFrameSettings::init(const RenderCamera &camera, double ambientPercent, ObjectTextureID paletteTextureID,
	ObjectTextureID lightTableTextureID, ObjectTextureID skyColorsTextureID,
	ObjectTextureID thunderstormColorsTextureID, int renderWidth, int renderHeight, int renderThreadsMode)
{
	this->camera = camera;
	this->ambientPercent = ambientPercent;
	this->paletteTextureID = paletteTextureID;
	this->lightTableTextureID = lightTableTextureID;
	this->skyColorsTextureID = skyColorsTextureID;
	this->thunderstormColorsTextureID = thunderstormColorsTextureID;
	this->renderWidth = renderWidth;
	this->renderHeight = renderHeight;
	this->renderThreadsMode = renderThreadsMode;
}
