#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include "../Assets/ArenaTypes.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector3.h"
#include "../Utilities/Color.h"
#include "../Utilities/Palette.h"
#include "../Voxels/VoxelUtils.h"

struct BoundingBox3D;
struct RenderCamera;

namespace RendererUtils
{
	constexpr double NEAR_PLANE = 0.01;
	constexpr double FAR_PLANE = 5000.0;

	RenderCamera makeCamera(const ChunkInt2 &chunk, const Double3 &point, const Double3 &direction,
		Degrees fovY, double aspectRatio, bool tallPixelCorrection);

	// Gets the number of render threads to use based on the given mode.
	int getRenderThreadsFromMode(int mode);

	// Gets the y-shear value of the camera based on the Y angle relative to the horizon
	// and the zoom of the camera (dependent on vertical field of view).
	double getYShear(Radians angleRadians, double zoom);

	// Converts a 3D point or vector in world space to camera space (where Z distance to vertices is relevant).
	// The W component of the point/vector matters (point=1, vector=0)!
	Double4 worldSpaceToCameraSpace(const Double4 &point, const Matrix4d &view);

	// Projects a 3D point or vector in camera space to clip space (homogeneous coordinates; does not divide by W).
	Double4 cameraSpaceToClipSpace(const Double4 &point, const Matrix4d &projection);

	// Projects a 3D point or vector in world space to clip space (homogeneous coordinates; does not divide by W).
	// The given transformation matrix is the product of a model, view, and perspective matrix. This function
	// combines the camera space step for convenience.
	Double4 worldSpaceToClipSpace(const Double4 &point, const Matrix4d &transform);

	// Converts a point in homogeneous coordinates to normalized device coordinates by dividing by W.
	Double3 clipSpaceToNDC(const Double4 &point);

	// Converts a point in normalized device coordinates to screen space (pixel coordinates with fractions in
	// the decimals; the space expected by pixel shading). In other 3D engines this extra step might not be needed
	// but I think I'm doing something different, can't remember.
	// - In theory, this would return an Int2+double later on if sub-pixel precision worked with integers instead.
	Double3 ndcToScreenSpace(const Double3 &point, double frameWidth, double frameHeight);

	// Gets the pixel coordinate with the nearest available pixel center based on the projected
	// value and some bounding rule. This is used to keep integer drawing ranges clamped in such
	// a way that they never allow sampling of texture coordinates outside of the 0->1 range.
	int getLowerBoundedPixel(double projected, int frameDim);
	int getUpperBoundedPixel(double projected, int frameDim);

	// Creates a rotation matrix for drawing latitude-correct distant space objects.
	Matrix4d getLatitudeRotation(double latitude);

	// Creates a rotation matrix for drawing distant space objects relative to the time of day.
	Matrix4d getTimeOfDayRotation(double daytimePercent);

	// Gets the palette index of the color that most closely matches the given one.
	int getNearestPaletteColorIndex(const Color &color, const Palette &palette);

	// Writes out visibility results for the bounding box test against the given camera frustum.
	void getBBoxVisibilityInFrustum(const BoundingBox3D &bbox, const RenderCamera &camera,
		bool *outIsCompletelyVisible, bool *outIsCompletelyInvisible);
}

#endif
