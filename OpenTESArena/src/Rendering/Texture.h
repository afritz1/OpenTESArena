#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>

#include "../Math/Float4.h"

// With the change to TextureReferences, this class might be obsolete. Textures
// are copied to the kernel buffers in the CLProgram, so there isn't really an
// in-between or a use for this class on the host.


// They are "Texture" in the host, but upon copying to the client, they are put into the 
// correct array based on their maximum side length, and the client picks which array to 
// index into based on the texture's "TextureSizeType" enum, and the width and height for 
// rendering would be obtained from the chosen texture's struct; this would need to have 
// N different struct branches, for each of the N different size types.

// A "TextureReference" struct with the type and index of the referenced texture would 
// need to be given to all objects that reference a texture; the type would determine which 
// array to pick from, and the index would pick from that array. I.e.:
// if (ref.type == SMALL) { get &small[ref.index].pixels; width = ...; height = ...; }
// else if (ref.type == MEDIUM) { get &medium[ref.index].pixels; width = ...; height = ...; }
// and so on. There would probably be some kind of enum mappings in the host to match objects 
// with the type and index they need so that they can be properly accessed in the kernel.

// ---------------------------------------------

// Perhaps walls should have their own exclusive texture type, "WallTexture", since they're 
// all going to be 64x64, and could be left alone in that regard. Modding aside, there's no 
// reason for any walls to have dimensions other than 64x64. At least the textures in Arena
// have 16x more pixels than Minecraft textures.

class Texture
{
private:
	// Vector4f is a fast format for the kernel to grab from, though hardly space-efficient.
	std::vector<Float4f> diffuse;
	int width, height;
public:
	// Only one constructor; for loading from an SDL_Surface or Surface.
	Texture(unsigned int *pixels, int width, int height);
	~Texture();

	int getWidth() const;
	int getHeight() const;
	const Float4f *getPixels() const;
};

#endif
