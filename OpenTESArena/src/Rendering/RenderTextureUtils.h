#ifndef RENDER_TEXTURE_UTILS_H
#define RENDER_TEXTURE_UTILS_H

// Common texture handles allocated by a renderer for a user when they want a new texture in the
// internal renderer format.

using ObjectTextureID = int; // For all scene geometry (voxels/entities/sky/particles).
using ObjectMaterialID = int; // For more complex shading of objects.

using UiTextureID = int; // For all UI textures.

class Renderer;

struct LockedTexture
{
	void *texels;
	bool isTrueColor;

	LockedTexture(void *texels, bool isTrueColor);

	bool isValid();
};

// Non-owning container of texture IDs for shading.
struct ObjectMaterial
{
	ObjectTextureID id0; // Main texture.
	ObjectTextureID id1; // Optional texture.

	ObjectMaterial(ObjectTextureID id0, ObjectTextureID id1);
	ObjectMaterial(ObjectTextureID id);
	ObjectMaterial();

	void init(ObjectTextureID id0, ObjectTextureID id1);
	void init(ObjectTextureID id);
};

// Owning reference to an object texture ID.
class ScopedObjectTextureRef
{
private:
	ObjectTextureID id;
	Renderer *renderer;
	int width, height;

	void setDims();
public:
	ScopedObjectTextureRef(ObjectTextureID id, Renderer &renderer);
	ScopedObjectTextureRef();
	ScopedObjectTextureRef(ScopedObjectTextureRef &&other);
	~ScopedObjectTextureRef();

	ScopedObjectTextureRef &operator=(ScopedObjectTextureRef &&other);

	void init(ObjectTextureID id, Renderer &renderer);

	ObjectTextureID get() const;
	int getWidth() const;
	int getHeight() const;

	// Texture updating functions.
	LockedTexture lockTexels();
	void unlockTexels();

	void destroy();
};

// Owning reference to an object material ID.
class ScopedObjectMaterialRef
{
private:
	ObjectMaterialID id;
	Renderer *renderer;
public:
	ScopedObjectMaterialRef(ObjectMaterialID id, Renderer &renderer);
	ScopedObjectMaterialRef();
	ScopedObjectMaterialRef(ScopedObjectMaterialRef &&other);
	~ScopedObjectMaterialRef();

	ScopedObjectMaterialRef &operator=(ScopedObjectMaterialRef &&other);

	void init(ObjectMaterialID id, Renderer &renderer);

	ObjectMaterialID get() const;

	void destroy();
};

// Owning reference to a UI texture ID.
class ScopedUiTextureRef
{
private:
	UiTextureID id;
	Renderer *renderer;
	int width, height;

	void setDims();
public:
	ScopedUiTextureRef(UiTextureID id, Renderer &renderer);
	ScopedUiTextureRef();
	ScopedUiTextureRef(ScopedUiTextureRef &&other);
	~ScopedUiTextureRef();

	ScopedUiTextureRef &operator=(ScopedUiTextureRef &&other);

	void init(UiTextureID id, Renderer &renderer);

	UiTextureID get() const;
	int getWidth() const;
	int getHeight() const;

	// Texture updating functions. The returned pointer allows for changing any texels in the texture.
	uint32_t *lockTexels();
	void unlockTexels();
};

#endif
