#ifndef AUTOMAP_PANEL_H
#define AUTOMAP_PANEL_H

#include <string>

#include "Panel.h"
#include "../Math/Vector2.h"
#include "../Media/TextureUtils.h"
#include "../UI/Button.h"
#include "../UI/TextBox.h"
#include "../Voxels/VoxelUtils.h"

// @todo: be able to click somewhere inside the drawable area of the automap and get a 2D voxel
// coordinate in world space for attaching a note to. Store the note in GameState or something.

// @todo: maybe split the automap into one texture per chunk
// - the only worry with this is SDL rendering imprecision, resulting in 1 pixel gaps between chunks.
// - Texture makeChunkTexture(??Int chunkX, ??Int chunkY);
// - Int2 getChunkPixelPosition(??Int chunkX, ??Int chunkY); // position on-screen in original render coords
// - just get the surrounding 3x3 chunks. Does it really matter that it's 2x2 like the original game?

class ChunkManager;
class Renderer;

class AutomapPanel : public Panel
{
private:
	TextBox locationTextBox;
	Button<Game&> backToGameButton;
	ScopedUiTextureRef mapTextureRef, backgroundTextureRef, cursorTextureRef;

	// XZ coordinate offset in automap space, stored as a real so scroll position can be sub-pixel.
	Double2 automapOffset;
public:
	AutomapPanel(Game &game);
	~AutomapPanel() override;

	bool init(const CoordDouble3 &playerCoord, const VoxelDouble2 &playerDirection,
		const ChunkManager &chunkManager, const std::string &locationName);
};

#endif
