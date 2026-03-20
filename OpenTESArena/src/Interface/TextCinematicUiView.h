#ifndef TEXT_CINEMATIC_UI_VIEW_H
#define TEXT_CINEMATIC_UI_VIEW_H

#include <string>

#include "../Rendering/RenderTextureUtils.h"

#include "components/utilities/Buffer.h"

class Renderer;
class TextureManager;

namespace TextCinematicUiView
{
	Buffer<UiTextureID> allocAnimationTextures(const std::string &animFilename, TextureManager &textureManager, Renderer &renderer);
}

#endif
