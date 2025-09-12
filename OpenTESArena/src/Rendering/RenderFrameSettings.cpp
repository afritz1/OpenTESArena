#include "RenderFrameSettings.h"

RenderFrameSettings::RenderFrameSettings()
{
	this->clearColor = Colors::Black;
	this->ambientPercent = 0.0;
	this->visibleLightsBufferID = -1;
	this->visibleLightCount = 0;
	this->screenSpaceAnimPercent = 0.0;
	this->paletteTextureID = -1;
	this->lightTableTextureID = -1;
	this->ditherTextureID = -1;
	this->skyBgTextureID = -1;
	this->renderThreadsMode = -1;
	this->ditheringMode = static_cast<DitheringMode>(-1);
}

void RenderFrameSettings::init(Color clearColor, double ambientPercent, UniformBufferID visibleLightsBufferID, int visibleLightCount,
	double screenSpaceAnimPercent, ObjectTextureID paletteTextureID, ObjectTextureID lightTableTextureID, ObjectTextureID ditherTextureID,
	ObjectTextureID skyBgTextureID, int renderThreadsMode, DitheringMode ditheringMode)
{
	this->clearColor = clearColor;
	this->ambientPercent = ambientPercent;
	this->visibleLightsBufferID = visibleLightsBufferID;
	this->visibleLightCount = visibleLightCount;
	this->screenSpaceAnimPercent = screenSpaceAnimPercent;
	this->paletteTextureID = paletteTextureID;
	this->lightTableTextureID = lightTableTextureID;
	this->ditherTextureID = ditherTextureID;
	this->skyBgTextureID = skyBgTextureID;
	this->renderThreadsMode = renderThreadsMode;
	this->ditheringMode = ditheringMode;
}
