#include "Renderer.h"
#include "RenderTextureUtils.h"

#include "components/debug/Debug.h"
#include "RenderBuffer.h"

LockedTexture::LockedTexture()
{
	this->width = 0;
	this->height = 0;
	this->bytesPerTexel = 0;
}

LockedTexture::LockedTexture(Span<std::byte> texels, int width, int height, int bytesPerTexel)
{
	this->texels = texels;
	this->width = width;
	this->height = height;
	this->bytesPerTexel = bytesPerTexel;
}

bool LockedTexture::isValid()
{
	return this->texels.isValid();
}

Span2D<uint8_t> LockedTexture::getTexels8()
{
	DebugAssert(this->bytesPerTexel == 1);
	return Span2D<uint8_t>(reinterpret_cast<uint8_t*>(this->texels.begin()), this->width, this->height);
}

Span2D<uint32_t> LockedTexture::getTexels32()
{
	DebugAssert(this->bytesPerTexel == 4);
	return Span2D<uint32_t>(reinterpret_cast<uint32_t*>(this->texels.begin()), this->width, this->height);
}

ScopedObjectTextureRef::ScopedObjectTextureRef(ObjectTextureID id, Renderer &renderer)
{
	DebugAssert(id >= 0);
	this->id = id;
	this->renderer = &renderer;
	this->setDims();
}

ScopedObjectTextureRef::ScopedObjectTextureRef()
{
	this->id = -1;
	this->renderer = nullptr;
	this->width = -1;
	this->height = -1;
}

ScopedObjectTextureRef::ScopedObjectTextureRef(ScopedObjectTextureRef &&other)
{
	this->id = other.id;
	this->renderer = other.renderer;
	this->width = other.width;
	this->height = other.height;
	other.id = -1;
	other.renderer = nullptr;
	other.width = -1;
	other.height = -1;
}

ScopedObjectTextureRef::~ScopedObjectTextureRef()
{
	this->destroy();
}

ScopedObjectTextureRef &ScopedObjectTextureRef::operator=(ScopedObjectTextureRef &&other)
{
	if (this != &other)
	{
		if (this->id >= 0)
		{
			this->destroy();
		}

		this->id = other.id;
		this->renderer = other.renderer;
		this->width = other.width;
		this->height = other.height;
		other.id = -1;
		other.renderer = nullptr;
		other.width = -1;
		other.height = -1;
	}

	return *this;
}

void ScopedObjectTextureRef::init(ObjectTextureID id, Renderer &renderer)
{
	if (this->id >= 0)
	{
		this->destroy();
	}

	DebugAssert(id >= 0);
	this->id = id;
	this->renderer = &renderer;
	this->setDims();
}

void ScopedObjectTextureRef::setDims()
{
	const std::optional<Int2> dims = this->renderer->tryGetObjectTextureDims(this->id);
	if (!dims.has_value())
	{
		DebugCrash("Couldn't get object texture dimensions (ID " + std::to_string(this->id) + ").");
	}

	this->width = dims->x;
	this->height = dims->y;
}

ObjectTextureID ScopedObjectTextureRef::get() const
{
	return this->id;
}

int ScopedObjectTextureRef::getWidth() const
{
	return this->width;
}

int ScopedObjectTextureRef::getHeight() const
{
	return this->height;
}

LockedTexture ScopedObjectTextureRef::lockTexels()
{
	return this->renderer->lockObjectTexture(this->id);
}

void ScopedObjectTextureRef::unlockTexels()
{
	this->renderer->unlockObjectTexture(this->id);
}

void ScopedObjectTextureRef::destroy()
{
	if (this->renderer != nullptr)
	{
		this->renderer->freeObjectTexture(this->id);
		this->renderer = nullptr;
		this->id = -1;
		this->width = -1;
		this->height = -1;
	}
}

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

ScopedUiTextureRef::ScopedUiTextureRef(ScopedUiTextureRef &&other)
{
	this->id = other.id;
	this->renderer = other.renderer;
	this->width = other.width;
	this->height = other.height;
	other.id = -1;
	other.renderer = nullptr;
	other.width = -1;
	other.height = -1;
}

ScopedUiTextureRef::~ScopedUiTextureRef()
{
	if (this->renderer != nullptr)
	{
		this->renderer->freeUiTexture(this->id);
	}
}

ScopedUiTextureRef &ScopedUiTextureRef::operator=(ScopedUiTextureRef &&other)
{
	if (this != &other)
	{
		this->id = other.id;
		this->renderer = other.renderer;
		this->width = other.width;
		this->height = other.height;
		other.id = -1;
		other.renderer = nullptr;
		other.width = -1;
		other.height = -1;
	}

	return *this;
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

Int2 ScopedUiTextureRef::getDimensions() const
{
	return Int2(this->width, this->height);
}

LockedTexture ScopedUiTextureRef::lockTexels()
{
	return this->renderer->lockUiTexture(this->id);
}

void ScopedUiTextureRef::unlockTexels()
{
	this->renderer->unlockUiTexture(this->id);
}
