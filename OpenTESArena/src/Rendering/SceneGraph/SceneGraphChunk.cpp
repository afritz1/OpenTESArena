#include "SceneGraphChunk.h"
#include "../../World/ChunkUtils.h"

void SceneGraphChunk::init(const ChunkInt2 &position, int height)
{
	this->position = position;
	this->voxels.init(ChunkUtils::CHUNK_DIM, height, ChunkUtils::CHUNK_DIM);
}
