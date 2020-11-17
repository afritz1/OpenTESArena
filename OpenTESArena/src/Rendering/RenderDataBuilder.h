#ifndef RENDER_DATA_BUILDER_H
#define RENDER_DATA_BUILDER_H

#include "RenderCamera.h"
#include "RenderDefinitionGroup.h"
#include "RenderInstanceGroup.h"

// Generates bulk render data from gameplay data to be passed to a renderer.

namespace RenderDataBuilder
{
	// @todo: pass gameplay data as parameters
	RenderCamera makeCamera();
	RenderDefinitionGroup makeDefinitions();
	RenderInstanceGroup makeInstances();
}

#endif
