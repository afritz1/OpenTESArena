#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include <string>

#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Media/TextureUtils.h"
#include "../UI/Button.h"
#include "../UI/Texture.h"
#include "../World/VoxelUtils.h"

// @todo: be able to click somewhere inside the drawable area of the automap and get a 2D voxel
// coordinate in world space for attaching a note to. Store the note in GameState or something.

// @todo: maybe split the automap into one texture per chunk
// - the only worry with this is SDL rendering imprecision, resulting in 1 pixel gaps between chunks.
// - Texture makeChunkTexture(??Int chunkX, ??Int chunkY);
// - Int2 getChunkPixelPosition(??Int chunkX, ??Int chunkY); // position on-screen in original render coords
// - just get the surrounding 3x3 chunks. Does it really matter that it's 2x2 like the original game?

class Chunk;
class ChunkManager;
class Color;
class Renderer;
class TextBox;
class TransitionDefinition;
class VoxelDefinition;

enum class CardinalDirectionName;

class AutomapPanel : public Panel
{
private:
	std::unique_ptr<TextBox> locationTextBox;
	Button<Game&> backToGameButton;
	Texture mapTexture;
	TextureBuilderID backgroundTextureBuilderID;
	PaletteID backgroundPaletteID;

	// XZ coordinate offset in automap space, stored as a real so scroll position can be sub-pixel.
	Double2 automapOffset;

	// Listen for when the LMB is held on a compass direction.
	void handleMouse(double dt);

	void drawTooltip(const std::string &text, Renderer &renderer);
public:
	AutomapPanel(Game &game);
	~AutomapPanel() override = default;

	bool init(const CoordDouble3 &playerCoord, const VoxelDouble2 &playerDirection,
		const ChunkManager &chunkManager, const std::string &locationName);

	virtual std::optional<CursorData> getCurrentCursor() const override;
	virtual void handleEvent(const SDL_Event &e) override;
	virtual void tick(double dt) override;
	virtual void render(Renderer &renderer) override;
};

#endif
