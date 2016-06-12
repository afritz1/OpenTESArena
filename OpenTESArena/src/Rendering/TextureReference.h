#ifndef TEXTURE_REFERENCE_H
#define TEXTURE_REFERENCE_H

// This class should be in the host so it can maintain the references. The kernel
// will use it during rendering.

// Instead of materials having to branch on a size type, they have a texture type
// (enum) that indexes into the texture reference array, which then points to the
// beginning of the relevant pixels in the giant float4 pixel array. Since all the 
// pixels of each picture are stored sequentially, it could just be implemented in 
// the kernel as a "const __global float4 *pixels". No need to complicate things 
// by only using just floats or float3's.

// The number of texture index objects would be equivalent to the number of texture 
// types used, and the texture type of the material would be the index into the 
// texture index object array.

class TextureReference
{
private:
	// Offset is the number of float4's to skip.
	int offset, width, height;
public:
	TextureReference(int offset, int width, int height);
	~TextureReference();

	int getOffset() const;
	int getWidth() const;
	int getHeight() const;
};

#endif
