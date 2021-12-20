#include <algorithm>
#include <cmath>
#include <limits>

#include "ArenaRenderUtils.h"
#include "LegacyRendererUtils.h"
#include "RenderCamera.h"
#include "RendererUtils.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "SoftwareRenderer.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Media/Color.h"
#include "../Media/Palette.h"
#include "../Media/TextureBuilder.h"
#include "../World/ChunkUtils.h"

#include "components/debug/Debug.h"

namespace swConstants
{
	constexpr double NEAR_PLANE = 0.1;
	constexpr double FAR_PLANE = 100.0;
}

// Internal geometry types/functions.
namespace swGeometry
{
	struct TriangleClipResult
	{
		int triangleCount = 0;
		RenderTriangle triangles[2];

		static TriangleClipResult zero()
		{
			TriangleClipResult result;
			result.triangleCount = 0;
			return result;
		}

		static TriangleClipResult one(const RenderTriangle &triangle)
		{
			TriangleClipResult result;
			result.triangleCount = 1;
			result.triangles[0] = triangle;
			return result;
		}

		static TriangleClipResult two(const RenderTriangle &triangleA, const RenderTriangle &triangleB)
		{
			TriangleClipResult result;
			result.triangleCount = 2;
			result.triangles[0] = triangleA;
			result.triangles[1] = triangleB;
			return result;
		}
	};

	TriangleClipResult ClipTriangle(const RenderTriangle &triangle, const Double3 &planePoint, const Double3 &planeNormal)
	{
		// @todo
		return TriangleClipResult::zero();
	}

	std::vector<RenderTriangle> MakeDebugCube(const Double3 &point)
	{
		std::vector<RenderTriangle> triangles;

		auto p = [&point](double x, double y, double z)
		{
			return point + Double3(x, y, z);
		};

		// Cube
		// X=0
		triangles.emplace_back(RenderTriangle(p(0.0, 1.0, 0.0), p(0.0, 0.0, 0.0), p(0.0, 0.0, 1.0)));
		triangles.emplace_back(RenderTriangle(p(0.0, 0.0, 1.0), p(0.0, 1.0, 1.0), p(0.0, 1.0, 0.0)));
		// X=1
		triangles.emplace_back(RenderTriangle(p(1.0, 1.0, 1.0), p(1.0, 0.0, 1.0), p(1.0, 0.0, 0.0)));
		triangles.emplace_back(RenderTriangle(p(1.0, 0.0, 0.0), p(1.0, 1.0, 0.0), p(1.0, 1.0, 1.0)));
		// Y=0
		triangles.emplace_back(RenderTriangle(p(1.0, 0.0, 1.0), p(0.0, 0.0, 1.0), p(0.0, 0.0, 0.0)));
		triangles.emplace_back(RenderTriangle(p(0.0, 0.0, 0.0), p(1.0, 0.0, 0.0), p(1.0, 0.0, 1.0)));
		// Y=1
		triangles.emplace_back(RenderTriangle(p(1.0, 1.0, 0.0), p(0.0, 1.0, 0.0), p(0.0, 1.0, 1.0)));
		triangles.emplace_back(RenderTriangle(p(0.0, 1.0, 1.0), p(1.0, 1.0, 1.0), p(1.0, 1.0, 0.0)));
		// Z=0
		triangles.emplace_back(RenderTriangle(p(1.0, 1.0, 0.0), p(1.0, 0.0, 0.0), p(0.0, 0.0, 0.0)));
		triangles.emplace_back(RenderTriangle(p(0.0, 0.0, 0.0), p(0.0, 1.0, 0.0), p(1.0, 1.0, 0.0)));
		// Z=1
		triangles.emplace_back(RenderTriangle(p(0.0, 1.0, 1.0), p(0.0, 0.0, 1.0), p(1.0, 0.0, 1.0)));
		triangles.emplace_back(RenderTriangle(p(1.0, 0.0, 1.0), p(1.0, 1.0, 1.0), p(0.0, 1.0, 1.0)));

		return triangles;
	}

	std::vector<RenderTriangle> MakeDebugMesh1()
	{
		return MakeDebugCube(Double3::Zero);
	}

	std::vector<RenderTriangle> MakeDebugMesh2()
	{
		std::vector<RenderTriangle> triangles;

		for (int y = 0; y < 3; y += 2)
		{
			const Double3 point(
				24.0,
				static_cast<double>(y),
				0.0);
			std::vector<RenderTriangle> cubeTriangles = MakeDebugCube(point);
			triangles.insert(triangles.end(), cubeTriangles.begin(), cubeTriangles.end());
		}

		return triangles;
	}

	std::vector<RenderTriangle> MakeDebugMesh3()
	{
		std::vector<RenderTriangle> triangles;

		for (int z = 0; z < 64; z += 2)
		{
			for (int x = 0; x < 64; x += 2)
			{
				const Double3 point(
					static_cast<double>(x),
					0.0,
					static_cast<double>(z));
				std::vector<RenderTriangle> cubeTriangles = MakeDebugCube(point);
				triangles.insert(triangles.end(), cubeTriangles.begin(), cubeTriangles.end());
			}
		}

		return triangles;
	}
}

// Rendering functions, per-pixel work.
namespace swRender
{
	void DrawDebugRGB(const RenderCamera &camera, BufferView2D<uint32_t> &colorBuffer)
	{
		const int frameBufferWidth = colorBuffer.getWidth();
		const int frameBufferHeight = colorBuffer.getHeight();
		uint32_t *colorBufferPtr = colorBuffer.get();

		const Double3 cameraRightScaledDir = camera.right * camera.aspectRatio;

		for (int y = 0; y < frameBufferHeight; y++)
		{
			const double yPercent = (static_cast<double>(y) + 0.50) / static_cast<double>(frameBufferHeight);

			for (int x = 0; x < frameBufferWidth; x++)
			{
				const double xPercent = (static_cast<double>(x) + 0.50) / static_cast<double>(frameBufferWidth);

				const Double3 pixelDir = ((camera.forward - cameraRightScaledDir + camera.up) +
					(cameraRightScaledDir * (xPercent * 2.0)) -
					(camera.up * (yPercent * 2.0))).normalized();

				const Double3 pixelDirClamped(
					std::max(pixelDir.x, 0.0),
					std::max(pixelDir.y, 0.0),
					std::max(pixelDir.z, 0.0));

				const Color color(
					static_cast<uint8_t>(pixelDirClamped.x * 255.0),
					static_cast<uint8_t>(pixelDirClamped.y * 255.0),
					static_cast<uint8_t>(pixelDirClamped.z * 255.0));

				const uint32_t outputColor = color.toARGB();
				const int outputIndex = x + (y * frameBufferWidth);
				colorBufferPtr[outputIndex] = outputColor;
			}
		}
	}

	void ClearFrameBuffers(uint32_t clearColor, BufferView2D<uint32_t> &colorBuffer, BufferView2D<double> &depthBuffer)
	{
		colorBuffer.fill(clearColor);
		depthBuffer.fill(std::numeric_limits<double>::infinity());
	}

	void RasterizeTriangles(const BufferView<const RenderTriangle> &triangles, const RenderCamera &camera,
		BufferView2D<uint32_t> &colorBuffer, BufferView2D<double> &depthBuffer)
	{
		const int frameBufferWidth = colorBuffer.getWidth();
		const int frameBufferHeight = colorBuffer.getHeight();
		const double frameBufferWidthReal = static_cast<double>(frameBufferWidth);
		const double frameBufferHeightReal = static_cast<double>(frameBufferHeight);

		const Double3 eye = VoxelUtils::chunkPointToNewPoint(camera.chunk, camera.point);
		const Matrix4d viewMatrix = Matrix4d::view(eye, camera.forward, camera.right, camera.up);
		const Matrix4d perspectiveMatrix = Matrix4d::perspective(camera.fovY, camera.aspectRatio,
			swConstants::NEAR_PLANE, swConstants::FAR_PLANE);

		constexpr double yShear = 0.0;

		const std::array<Double3, 6> colors =
		{
			Double3::fromRGB(Color::Red.toARGB()),
			Double3::fromRGB(Color::Green.toARGB()),
			Double3::fromRGB(Color::Blue.toARGB()),
			Double3::fromRGB(Color::Cyan.toARGB()),
			Double3::fromRGB(Color::Magenta.toARGB()),
			Double3::fromRGB(Color::Yellow.toARGB())
		};

		uint32_t *colorBufferPtr = colorBuffer.get();
		double *depthBufferPtr = depthBuffer.get();

		for (int i = 0; i < triangles.getCount(); i++)
		{
			const RenderTriangle &triangle = triangles.get(i);
			const Double3 &v0 = triangle.v0;
			const Double3 &v1 = triangle.v1;
			const Double3 &v2 = triangle.v2;

			// Discard back-facing.
			const Double3 v0ToEye = eye - v0;
			if (v0ToEye.dot(triangle.normal) <= 0.0)
			{
				continue;
			}

			const Double4 view0 = RendererUtils::worldSpaceToCameraSpace(Double4(v0, 1.0), viewMatrix);
			const Double4 view1 = RendererUtils::worldSpaceToCameraSpace(Double4(v1, 1.0), viewMatrix);
			const Double4 view2 = RendererUtils::worldSpaceToCameraSpace(Double4(v2, 1.0), viewMatrix);

			// Nearest and farthest Z values (note these may be negative - behind the camera).
			const double zMin = std::min(view0.z, std::min(view1.z, view2.z));
			const double zMax = std::max(view0.z, std::max(view1.z, view2.z));

			if ((zMin < swConstants::NEAR_PLANE) || (zMin > swConstants::FAR_PLANE) ||
				(zMax < swConstants::NEAR_PLANE) || (zMax > swConstants::FAR_PLANE))
			{
				continue;
			}

			const Double4 clip0 = RendererUtils::cameraSpaceToClipSpace(view0, perspectiveMatrix);
			const Double4 clip1 = RendererUtils::cameraSpaceToClipSpace(view1, perspectiveMatrix);
			const Double4 clip2 = RendererUtils::cameraSpaceToClipSpace(view2, perspectiveMatrix);
			const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clip0);
			const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clip1);
			const Double3 ndc2 = RendererUtils::clipSpaceToNDC(clip2);
			const Double3 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, yShear, frameBufferWidthReal, frameBufferHeightReal);
			const Double3 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, yShear, frameBufferWidthReal, frameBufferHeightReal);
			const Double3 screenSpace2 = RendererUtils::ndcToScreenSpace(ndc2, yShear, frameBufferWidthReal, frameBufferHeightReal);
			const Double2 screenSpace0_2D(screenSpace0.x, screenSpace0.y);
			const Double2 screenSpace1_2D(screenSpace1.x, screenSpace1.y);
			const Double2 screenSpace2_2D(screenSpace2.x, screenSpace2.y);
			const Double2 screenSpace01 = screenSpace1_2D - screenSpace0_2D;
			const Double2 screenSpace12 = screenSpace2_2D - screenSpace1_2D;
			const Double2 screenSpace20 = screenSpace0_2D - screenSpace2_2D;
			const Double2 screenSpace01Perp = screenSpace01.rightPerp();
			const Double2 screenSpace12Perp = screenSpace12.rightPerp();
			const Double2 screenSpace20Perp = screenSpace20.rightPerp();

			// Naive screen-space bounding box around triangle.
			const double xMin = std::min(screenSpace0.x, std::min(screenSpace1.x, screenSpace2.x));
			const double xMax = std::max(screenSpace0.x, std::max(screenSpace1.x, screenSpace2.x));
			const double yMin = std::min(screenSpace0.y, std::min(screenSpace1.y, screenSpace2.y));
			const double yMax = std::max(screenSpace0.y, std::max(screenSpace1.y, screenSpace2.y));
			const int xStart = RendererUtils::getLowerBoundedPixel(xMin, frameBufferWidth);
			const int xEnd = RendererUtils::getUpperBoundedPixel(xMax, frameBufferWidth);
			const int yStart = RendererUtils::getLowerBoundedPixel(yMin, frameBufferHeight);
			const int yEnd = RendererUtils::getUpperBoundedPixel(yMax, frameBufferHeight);

			const Double3 &color = colors[(i / 2) % static_cast<int>(colors.size())];
			const uint32_t colorInteger = color.toRGB();
			
			for (int y = yStart; y < yEnd; y++)
			{
				const double yScreenPercent = (static_cast<double>(y) + 0.50) / frameBufferHeightReal;

				for (int x = xStart; x < xEnd; x++)
				{
					const double xScreenPercent = (static_cast<double>(x) + 0.50) / frameBufferWidthReal;
					const Double2 pixelCenter(
						xScreenPercent * frameBufferWidthReal,
						yScreenPercent * frameBufferHeightReal);

					// See if pixel center is inside triangle.
					const bool inHalfSpace0 = MathUtils::isPointInHalfSpace(pixelCenter, screenSpace0_2D, screenSpace01Perp);
					const bool inHalfSpace1 = MathUtils::isPointInHalfSpace(pixelCenter, screenSpace1_2D, screenSpace12Perp);
					const bool inHalfSpace2 = MathUtils::isPointInHalfSpace(pixelCenter, screenSpace2_2D, screenSpace20Perp);
					if (inHalfSpace0 && inHalfSpace1 && inHalfSpace2)
					{
						const int outputIndex = x + (y * frameBufferWidth);
						colorBufferPtr[outputIndex] = colorInteger;
					}
				}
			}
		}
	}
}

void SoftwareRenderer::ObjectTexture::init8Bit(int width, int height)
{
	this->texels.init(width, height);
}

void SoftwareRenderer::ObjectTexture::initPalette(int count)
{
	this->paletteTexels.init(count);
}

void SoftwareRenderer::ObjectTexture::clear()
{
	this->texels.clear();
	this->paletteTexels.clear();
}

SoftwareRenderer::SoftwareRenderer()
{
	this->nextObjectTextureID = -1;
}

SoftwareRenderer::~SoftwareRenderer()
{

}

void SoftwareRenderer::init(const RenderInitSettings &settings)
{
	this->depthBuffer.init(settings.width, settings.height);
	this->nextObjectTextureID = 0;
}

void SoftwareRenderer::shutdown()
{
	this->depthBuffer.clear();
	this->objectTextures.clear();
	this->freedObjectTextureIDs.clear();
	this->nextObjectTextureID = -1;
}

bool SoftwareRenderer::isInited() const
{
	return this->depthBuffer.isValid();
}

void SoftwareRenderer::resize(int width, int height)
{
	this->depthBuffer.init(width, height);
	this->depthBuffer.fill(std::numeric_limits<double>::infinity());
}

bool SoftwareRenderer::tryCreateObjectTexture(int width, int height, bool isPalette, ObjectTextureID *outID)
{
	ObjectTexture texture;
	if (!isPalette)
	{
		texture.init8Bit(width, height);
		texture.texels.fill(0);
	}
	else
	{
		texture.initPalette(width * height);
		texture.paletteTexels.fill(0);
	}

	if (!this->freedObjectTextureIDs.empty())
	{
		*outID = this->freedObjectTextureIDs.back();
		this->freedObjectTextureIDs.pop_back();
		this->objectTextures[*outID] = std::move(texture);
	}
	else
	{
		*outID = this->nextObjectTextureID;
		this->nextObjectTextureID++;
		this->objectTextures.emplace_back(std::move(texture));
	}

	return true;
}

bool SoftwareRenderer::tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID)
{
	const int width = textureBuilder.getWidth();
	const int height = textureBuilder.getHeight();
	if (!this->tryCreateObjectTexture(width, height, false, outID))
	{
		DebugLogWarning("Couldn't create " + std::to_string(width) + "x" + std::to_string(height) + " object texture.");
		return false;
	}

	DebugAssertIndex(this->objectTextures, *outID);
	ObjectTexture &texture = this->objectTextures[*outID];

	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
		std::copy(palettedTexture.texels.get(), palettedTexture.texels.end(), texture.texels.get());
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		DebugLogWarning("True color texture (dimensions " + std::to_string(width) + "x" + std::to_string(height) + ") not supported.");
		texture.texels.fill(0);
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}

	return true;
}

LockedTexture SoftwareRenderer::lockObjectTexture(ObjectTextureID id)
{
	DebugAssertIndex(this->objectTextures, id);
	ObjectTexture &texture = this->objectTextures[id];
	if (texture.texels.isValid())
	{
		return LockedTexture(texture.texels.get(), false);
	}
	else if (texture.paletteTexels.isValid())
	{
		return LockedTexture(texture.paletteTexels.get(), true);
	}
	else
	{
		DebugNotImplemented();
		return LockedTexture(nullptr, false);
	}
}

void SoftwareRenderer::unlockObjectTexture(ObjectTextureID id)
{
	// Do nothing; any writes are already in RAM.
	static_cast<void>(id);
}

void SoftwareRenderer::freeObjectTexture(ObjectTextureID id)
{
	DebugAssertIndex(this->objectTextures, id);
	ObjectTexture &texture = this->objectTextures[id];
	texture.clear();
	this->freedObjectTextureIDs.emplace_back(id);
}

std::optional<Int2> SoftwareRenderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	DebugAssertIndex(this->objectTextures, id);
	const ObjectTexture &texture = this->objectTextures[id];
	return Int2(texture.texels.getWidth(), texture.texels.getHeight());
}

bool SoftwareRenderer::tryGetEntitySelectionData(const Double2 &uv, ObjectTextureID textureID, bool pixelPerfect, bool *outIsSelected) const
{
	if (pixelPerfect)
	{
		// Get the texture list from the texture group at the given animation state and angle.
		DebugAssertIndex(this->objectTextures, textureID);
		const ObjectTexture &texture = this->objectTextures[textureID];
		const int textureWidth = texture.texels.getWidth();
		const int textureHeight = texture.texels.getHeight();

		const int textureX = static_cast<int>(uv.x * static_cast<double>(textureWidth));
		const int textureY = static_cast<int>(uv.y * static_cast<double>(textureHeight));

		if ((textureX < 0) || (textureX >= textureWidth) ||
			(textureY < 0) || (textureY >= textureHeight))
		{
			// Outside the texture; out of bounds.
			return false;
		}

		// Check if the texel is non-transparent.
		const uint8_t texel = texture.texels.get(textureX, textureY);
		*outIsSelected = texel != 0;
		return true;
	}
	else
	{
		// The entity's projected rectangle is hit if the texture coordinates are valid.
		const bool withinEntity = (uv.x >= 0.0) && (uv.x <= 1.0) && (uv.y >= 0.0) && (uv.y <= 1.0);
		*outIsSelected = withinEntity;
		return true;
	}
}

Double3 SoftwareRenderer::screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
	Degrees fovY, double aspect) const
{
	return LegacyRendererUtils::screenPointToRay(xPercent, yPercent, cameraDirection, fovY, aspect);
}

RendererSystem3D::ProfilerData SoftwareRenderer::getProfilerData() const
{
	const int renderWidth = this->depthBuffer.getWidth();
	const int renderHeight = this->depthBuffer.getHeight();

	// @todo
	const int threadCount = 1;
	const int potentiallyVisFlatCount = 0;
	const int visFlatCount = 0;
	const int visLightCount = 0;

	return ProfilerData(renderWidth, renderHeight, threadCount, potentiallyVisFlatCount, visFlatCount, visLightCount);
}

void SoftwareRenderer::submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings, uint32_t *outputBuffer)
{
	const int frameBufferWidth = this->depthBuffer.getWidth();
	const int frameBufferHeight = this->depthBuffer.getHeight();
	BufferView2D<uint32_t> colorBufferView(outputBuffer, frameBufferWidth, frameBufferHeight);
	BufferView2D<double> depthBufferView(this->depthBuffer.get(), frameBufferWidth, frameBufferHeight);

	// Palette for 8-bit -> 32-bit color conversion.
	//const ObjectTexture &paletteTexture = this->objectTextures[settings.paletteTextureID];

	const uint32_t clearColor = Color::Gray.toARGB();
	swRender::ClearFrameBuffers(clearColor, colorBufferView, depthBufferView);
	swRender::DrawDebugRGB(camera, colorBufferView);

	const std::vector<RenderTriangle> triangles = swGeometry::MakeDebugMesh2();
	const BufferView<const RenderTriangle> trianglesView(triangles.data(), static_cast<int>(triangles.size()));
	swRender::RasterizeTriangles(trianglesView, camera, colorBufferView, depthBufferView);
}

void SoftwareRenderer::present()
{
	// Do nothing for now, might change later.
}
