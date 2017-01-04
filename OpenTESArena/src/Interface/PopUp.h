#ifndef POP_UP_H
#define POP_UP_H

// A static class for generating various pop-up textures.

class Renderer;
class TextureManager;

enum class PopUpType;

struct SDL_Texture;

class PopUp
{
private:
	PopUp() = delete;
	PopUp(const PopUp&) = delete;
	~PopUp() = delete;
public:
	static SDL_Texture *create(PopUpType type, int width, int height,
		TextureManager &textureManager, Renderer &renderer);
};

#endif
