#include "RendererInterface.h"
#include "RenderTextureUtils.h"

ScopedVoxelTextureRef::ScopedVoxelTextureRef(VoxelTextureID id, RendererInterface &rendererInterface)
{
	this->id = id;
	this->rendererInterface = &rendererInterface;
}

ScopedVoxelTextureRef::~ScopedVoxelTextureRef()
{
	this->rendererInterface->freeVoxelTexture(this->id);
}

VoxelTextureID ScopedVoxelTextureRef::get() const
{
	return this->id;
}

ScopedEntityTextureRef::ScopedEntityTextureRef(EntityTextureID id, RendererInterface &rendererInterface)
{
	this->id = id;
	this->rendererInterface = &rendererInterface;
}

ScopedEntityTextureRef::~ScopedEntityTextureRef()
{
	this->rendererInterface->freeEntityTexture(this->id);
}

EntityTextureID ScopedEntityTextureRef::get() const
{
	return this->id;
}

ScopedSkyTextureRef::ScopedSkyTextureRef(SkyTextureID id, RendererInterface &rendererInterface)
{
	this->id = id;
	this->rendererInterface = &rendererInterface;
}

ScopedSkyTextureRef::~ScopedSkyTextureRef()
{
	this->rendererInterface->freeSkyTexture(this->id);
}

SkyTextureID ScopedSkyTextureRef::get() const
{
	return this->id;
}

ScopedUiTextureRef::ScopedUiTextureRef(UiTextureID id, RendererInterface &rendererInterface)
{
	this->id = id;
	this->rendererInterface = &rendererInterface;
}

ScopedUiTextureRef::~ScopedUiTextureRef()
{
	this->rendererInterface->freeUiTexture(this->id);
}

UiTextureID ScopedUiTextureRef::get() const
{
	return this->id;
}
