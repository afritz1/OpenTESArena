#ifndef CHARACTER_SHEET_UI_VIEW_H
#define CHARACTER_SHEET_UI_VIEW_H

#include "../Math/Vector2.h"
#include "../Rendering/RenderTextureUtils.h"

struct TextureAsset;

class Game;
class Renderer;
class TextureManager;

namespace CharacterSheetUiView
{
	Int2 getBodyOffset(Game &game);
	Int2 getHeadOffset(Game &game);
	Int2 getShirtOffset(Game &game);
	Int2 getPantsOffset(Game &game);

	TextureAsset getPaletteTextureAsset();
	TextureAsset getBodyTextureAsset(Game &game);
	TextureAsset getHeadTextureAsset(Game &game);
	TextureAsset getShirtTextureAsset(Game &game);
	TextureAsset getPantsTextureAsset(Game &game);

	UiTextureID allocBodyTexture(Game &game);
	UiTextureID allocShirtTexture(Game &game);
	UiTextureID allocPantsTexture(Game &game);
	UiTextureID allocHeadTexture(Game &game);
}

namespace CharacterEquipmentUiView
{
	UiTextureID allocUpDownButtonTexture(TextureManager &textureManager, Renderer &renderer);
}

#endif
