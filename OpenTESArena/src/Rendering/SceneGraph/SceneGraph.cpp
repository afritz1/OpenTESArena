#include "SceneGraph.h"
#include "../../World/ChunkManager.h"

#include "components/debug/Debug.h"

const BufferView<const int> SceneGraph::getVisibleGeometry() const
{
    DebugUnhandledReturn(BufferView<const int>);
}

void SceneGraph::updateVoxels(const ChunkManager &chunkManager, double ceilingScale, double chasmAnimPercent)
{
    this->clearVoxels();

    // Compare chunk manager chunk count w/ our grid size and resize if needed
    if (this->chunkRenderInsts.size() != chunkManager.getChunkCount())
    {

    }

    DebugNotImplemented();
}

void SceneGraph::updateEntities(const EntityManager &entityManager, bool nightLightsAreActive, bool playerHasLight)
{
    this->clearEntities();
    DebugNotImplemented();
}

void SceneGraph::updateSky(const SkyInstance &skyInst, double daytimePercent, double latitude)
{
    this->clearSky();
    DebugNotImplemented();
}

void SceneGraph::updateVisibleGeometry(const RenderCamera &camera)
{
    // @todo: clear current geometry/light/etc. buffers

    DebugNotImplemented();
}

void SceneGraph::clearVoxels()
{
    this->voxelRenderDefs.clear();
    this->chunkRenderDefs.clear();
    this->chunkRenderInsts.clear();
}

void SceneGraph::clearEntities()
{
    this->entityRenderDefs.clear();
    this->entityRenderInsts.clear();
}

void SceneGraph::clearSky()
{
    this->skyObjectRenderDefs.clear();
    this->skyObjectRenderInsts.clear();
}

void SceneGraph::clear()
{
    this->clearVoxels();
    this->clearEntities();
    this->clearSky();
}
