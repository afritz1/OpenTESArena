#include "RendererSystem2D.h"
#include "RendererSystem3D.h"
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
}

ScopedUiTextureRef::ScopedUiTextureRef(UiTextureID id, RendererSystem2D &rendererSystem)
{
	this->id = id;
	this->rendererSystem = &rendererSystem;
}

ScopedUiTextureRef::~ScopedUiTextureRef()
{
	this->rendererSystem->freeUiTexture(this->id);
}

UiTextureID ScopedUiTextureRef::get() const
{
	return this->id;
}*/
