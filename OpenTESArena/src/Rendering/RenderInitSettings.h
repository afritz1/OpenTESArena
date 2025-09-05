#ifndef RENDER_INIT_SETTINGS_H
#define RENDER_INIT_SETTINGS_H

#include <string>

#include "RenderShaderUtils.h"

struct Window;

struct RenderContextSettings
{
	const Window *window;
	bool enableValidationLayers;
	
	RenderContextSettings();
	
	void init(const Window *window, bool enableValidationLayers);
};

struct RenderInitSettings
{
	const Window *window;
	std::string dataFolderPath;

	int internalWidth, internalHeight;
	int renderThreadsMode;
	DitheringMode ditheringMode;
	
	RenderInitSettings();

	void init(const Window *window, const std::string &dataFolderPath, int internalWidth, int internalHeight, int renderThreadsMode,
		DitheringMode ditheringMode);
};

#endif
