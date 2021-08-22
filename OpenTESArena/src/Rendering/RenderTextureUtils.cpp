#include "Renderer.h"
#include "RenderTextureUtils.h"

#include "components/debug/Debug.h"

/*ScopedVoxelTextureRef::ScopedVoxelTextureRef(VoxelTextureID id, RendererSystem3D &rendererSystem)
{
	this->id = id;
	this->rendererSystem = &rendererSystem;
}

ScopedVoxelTextureRef::~ScopedVoxelTextureRef()
{
	this->rendererSystem->freeVoxelTexture(this->id);
}

VoxelTextureID ScopedVoxelTextureRef::get() const
{
	return this->id;
}

ScopedEntityTextureRef::ScopedEntityTextureRef(EntityTextureID id, RendererSystem3D &rendererSystem)
{
	this->id = id;
	this->rendererSystem = &rendererSystem;
}

ScopedEntityTextureRef::~ScopedEntityTextureRef()
{
	this->rendererSystem->freeEntityTexture(this->id);
}

EntityTextureID ScopedEntityTextureRef::get() const
{
	return this->id;
}

ScopedSkyTextureRef::ScopedSkyTextureRef(SkyTextureID id, RendererSystem3D &rendererSystem)
{
	this->id = id;
	this->rendererSystem = &rendererSystem;
}

ScopedSkyTextureRef::~ScopedSkyTextureRef()
{
	this->rendererSystem->freeSkyTexture(this->id);
}

SkyTextureID ScopedSkyTextureRef::get() const
{
	return this->id;
}*/

ScopedUiTextureRef::ScopedUiTextureRef(UiTextureID id, Renderer &renderer)
{
	DebugAssert(id >= 0);
	this->id = id;
	this->renderer = &renderer;
	this->setDims();
}

ScopedUiTextureRef::ScopedUiTextureRef()
{
	this->id = -1;
	this->renderer = nullptr;
	this->width = -1;
	this->height = -1;
}

ScopedUiTextureRef::~ScopedUiTextureRef()
{
	if (this->renderer != nullptr)
	{
		this->renderer->freeUiTexture(this->id);
	}
}

void ScopedUiTextureRef::init(UiTextureID id, Renderer &renderer)
{
	DebugAssert(this->id == -1);
	DebugAssert(this->renderer == nullptr);
	DebugAssert(id >= 0);
	this->id = id;
	this->renderer = &renderer;
	this->setDims();
}

void ScopedUiTextureRef::setDims()
{
	const std::optional<Int2> dims = this->renderer->tryGetUiTextureDims(this->id);
	if (!dims.has_value())
	{
		DebugCrash("Couldn't get UI texture dimensions (ID " + std::to_string(this->id) + ").");
	}

	this->width = dims->x;
	this->height = dims->y;
}

UiTextureID ScopedUiTextureRef::get() const
{
	return this->id;
}

int ScopedUiTextureRef::getWidth() const
{
	return this->width;
}

int ScopedUiTextureRef::getHeight() const
{
	return this->height;
}

uint32_t *ScopedUiTextureRef::lockTexels()
{
	return this->renderer->lockUiTexture(this->id);
}

void ScopedUiTextureRef::unlockTexels()
{
	this->renderer->unlockUiTexture(this->id);
}
