#ifndef RENDER_INIT_SETTINGS_H
#define RENDER_INIT_SETTINGS_H

#include <string>

#include "RenderShaderUtils.h"

struct SDL_Window;

struct RenderInitSettings
{
	SDL_Window *window;
	std::string dataFolderPath;

	int width, height;
	int renderThreadsMode;
	DitheringMode ditheringMode;

	void init(SDL_Window *window, const std::string &dataFolderPath, int width, int height, int renderThreadsMode, DitheringMode ditheringMode);
};

#endif
