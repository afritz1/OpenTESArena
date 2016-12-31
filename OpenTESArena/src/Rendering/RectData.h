#ifndef RECT_DATA_H
#define RECT_DATA_H

#include "Rect3D.h"

// Helper class for managing a rectangle and its texture in OpenCL device memory.

class RectData
{
private:
	Rect3D rect;
	int textureID;
public:
	RectData(const Rect3D &rect, int textureID);
	~RectData();

	const Rect3D &getRect() const;
	int getTextureID() const;
};

#endif
