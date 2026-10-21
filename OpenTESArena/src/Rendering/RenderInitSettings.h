#pragma once

#include <functional>
#include <string>

#include "RenderShaderUtils.h"
#include "../Math/Vector2.h"

struct Window;

using RenderInternalDimensionsFunc = std::function<Int2(const Window *window)>;

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

	RenderInternalDimensionsFunc internalDimsFunc;
	int renderThreadsMode;
	DitheringMode ditheringMode;
	
	RenderInitSettings();

	void init(const Window *window, const std::string &dataFolderPath, const RenderInternalDimensionsFunc &internalDimsFunc,
		int renderThreadsMode, DitheringMode ditheringMode);
};
