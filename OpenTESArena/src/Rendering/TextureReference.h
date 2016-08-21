#ifndef TEXTURE_REFERENCE_H
#define TEXTURE_REFERENCE_H

#include <cstdint>

// This class should be in the host so it can maintain the references. The kernel
// will use it during rendering.

class TextureReference
{
private:
	// Offset is the number of float4's to skip.
	int32_t offset;
	short width, height;
public:
	TextureReference(int32_t offset, short width, short height);
	~TextureReference();

	int32_t getOffset() const;
	short getWidth() const;
	short getHeight() const;
};

#endif
