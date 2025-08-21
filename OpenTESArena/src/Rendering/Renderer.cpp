#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "RenderBackendType.h"
#include "RenderBuffer.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "RenderTransform.h"
#include "Sdl2DSoft3DRenderBackend.h"
#include "VulkanRenderBackend.h"
#include "../Assets/TextureManager.h"
#include "../Math/MathUtils.h"
#include "../Math/Rect.h"
#include "../UI/CursorAlignment.h"
#include "../UI/GuiUtils.h"
#include "../UI/RenderSpace.h"
#include "../UI/Surface.h"
#include "../UI/UiCommand.h"
#include "../UI/UiDrawCall.h"
#include "../Utilities/Color.h"
#include "../Utilities/Platform.h"

#include "components/debug/Debug.h"
#include "components/utilities/String.h"

namespace
{
	constexpr double PHYSICS_DEBUG_MAX_DISTANCE = 4.0;
	constexpr double PHYSICS_DEBUG_MAX_DISTANCE_SQR = PHYSICS_DEBUG_MAX_DISTANCE * PHYSICS_DEBUG_MAX_DISTANCE;

	Int2 MakeInternalRendererDimensions(const Int2 &dimensions, double resolutionScale)
	{
		const double scaledWidthReal = static_cast<double>(dimensions.x) * resolutionScale;
		const double scaledHeightReal = static_cast<double>(dimensions.y) * resolutionScale;

		// Avoid off-by-one like 1079p.
		const int roundedWidth = static_cast<int>(std::round(scaledWidthReal));
		const int roundedHeight = static_cast<int>(std::round(scaledHeightReal));

		// Keep as a multiple of a power of 2 for SIMD-friendliness. Don't worry about skewing aspect ratio at low resolution.
		// The camera retains the projection so the result is taller/wider pixels.
		constexpr int alignment = RendererUtils::RESOLUTION_ALIGNMENT;
		constexpr int alignmentMask = ~(alignment - 1);
		const int alignedWidth = std::max(roundedWidth & alignmentMask, alignment);
		const int alignedHeight = std::max(roundedHeight & alignmentMask, alignment);

		return Int2(alignedWidth, alignedHeight);
	}

	static_assert(sizeof(RenderTransform) == sizeof(Matrix4d) * 3);
	constexpr int BytesPerTransformF = sizeof(RenderTransform) / 2;
	constexpr int BytesPerMatrix4f = sizeof(Matrix4f);

	void WriteRenderTransformFloat32(const RenderTransform &transform, std::byte *dstBytes)
	{
		const Matrix4f &translationF = RendererUtils::matrix4DoubleToFloat(transform.translation);
		const Matrix4f &rotationF = RendererUtils::matrix4DoubleToFloat(transform.rotation);
		const Matrix4f &scaleF = RendererUtils::matrix4DoubleToFloat(transform.scale);

		Span<const std::byte> srcTranslationBytes(reinterpret_cast<const std::byte*>(&translationF), BytesPerMatrix4f);
		Span<const std::byte> srcRotationBegin(reinterpret_cast<const std::byte*>(&rotationF), BytesPerMatrix4f);
		Span<const std::byte> srcScaleBegin(reinterpret_cast<const std::byte*>(&scaleF), BytesPerMatrix4f);
		Span<std::byte> dstTranslationBytes(dstBytes, BytesPerMatrix4f);
		Span<std::byte> dstRotationBytes(dstTranslationBytes.end(), BytesPerMatrix4f);
		Span<std::byte> dstScaleBytes(dstRotationBytes.end(), BytesPerMatrix4f);
		std::copy(srcTranslationBytes.begin(), srcTranslationBytes.end(), dstTranslationBytes.begin());
		std::copy(srcRotationBegin.begin(), srcRotationBegin.end(), dstRotationBytes.begin());
		std::copy(srcScaleBegin.begin(), srcScaleBegin.end(), dstScaleBytes.begin());
	}
}

RenderElement2D::RenderElement2D(UiTextureID id, Rect rect, Rect clipRect)
	: rect(rect), clipRect(clipRect)
{
	this->id = id;
}

RenderElement2D::RenderElement2D()
	: RenderElement2D(-1, Rect(), Rect()) { }

RenderDisplayMode::RenderDisplayMode(int width, int height, int refreshRate)
{
	this->width = width;
	this->height = height;
	this->refreshRate = refreshRate;
}

RendererProfilerData::RendererProfilerData()
{
	this->width = -1;
	this->height = -1;
	this->pixelCount = -1;
	this->threadCount = -1;
	this->drawCallCount = -1;
	this->presentedTriangleCount = -1;
	this->objectTextureCount = -1;
	this->objectTextureByteCount = -1;
	this->totalLightCount = -1;
	this->totalCoverageTests = -1;
	this->totalDepthTests = -1;
	this->totalColorWrites = -1;
	this->renderTime = 0.0;
	this->presentTime = 0.0;
}

void RendererProfilerData::init(int width, int height, int threadCount, int drawCallCount, int presentedTriangleCount,
	int objectTextureCount, int64_t objectTextureByteCount, int totalLightCount, int64_t totalCoverageTests, int64_t totalDepthTests,
	int64_t totalColorWrites, double renderTime, double presentTime)
{
	this->width = width;
	this->height = height;
	this->pixelCount = width * height;
	this->threadCount = threadCount;
	this->drawCallCount = drawCallCount;
	this->presentedTriangleCount = presentedTriangleCount;
	this->objectTextureCount = objectTextureCount;
	this->objectTextureByteCount = objectTextureByteCount;
	this->totalLightCount = totalLightCount;
	this->totalCoverageTests = totalCoverageTests;
	this->totalDepthTests = totalDepthTests;
	this->totalColorWrites = totalColorWrites;
	this->renderTime = renderTime;
	this->presentTime = presentTime;
}

Renderer::Renderer()
{
	this->window = nullptr;
}

Renderer::~Renderer()
{
	DebugLog("Closing.");

	if (this->backend != nullptr)
	{
		this->backend->shutdown();
		this->backend = nullptr;
	}

	this->window = nullptr;
}

bool Renderer::init(const Window *window, RenderBackendType backendType, const RenderResolutionScaleFunc &resolutionScaleFunc,
	int renderThreadsMode, DitheringMode ditheringMode, bool enableValidationLayers, const std::string &dataFolderPath)
{
	DebugLog("Initializing.");

	this->window = window;
	this->resolutionScaleFunc = resolutionScaleFunc;

	switch (backendType)
	{
	case RenderBackendType::Sdl2DSoft3D:
		this->backend = std::make_unique<Sdl2DSoft3DRenderBackend>();
		break;
	case RenderBackendType::Vulkan:
		this->backend = std::make_unique<VulkanRenderBackend>();
		break;
	default:
		DebugLogErrorFormat("Unrecognized render backend %d.", backendType);
		return false;
	}

	const Int2 viewDims = window->getViewDimensions();
	const double resolutionScale = resolutionScaleFunc();
	const Int2 internalRenderDims = MakeInternalRendererDimensions(viewDims, resolutionScale);

	RenderInitSettings initSettings;
	initSettings.init(window, dataFolderPath, internalRenderDims.x, internalRenderDims.y, renderThreadsMode, ditheringMode, enableValidationLayers);
	if (!this->backend->init(initSettings))
	{
		DebugLogErrorFormat("Couldn't init render backend %d.", backendType);
		this->backend->shutdown();
		this->backend = nullptr;
		return false;
	}

	return true;
}

Surface Renderer::getScreenshot() const
{
	return this->backend->getScreenshot();
}

const RendererProfilerData &Renderer::getProfilerData() const
{
	return this->profilerData;
}

void Renderer::resize(int width, int height)
{
	// The window's dimensions are resized automatically by SDL. The renderer's are not.
	const Int2 windowDims = this->window->getDimensions();
	const Int2 viewDims = this->window->getViewDimensions();
	const double resolutionScale = this->resolutionScaleFunc();
	const Int2 internalDims = MakeInternalRendererDimensions(viewDims, resolutionScale);
	this->backend->resize(windowDims.x, windowDims.y, internalDims.x, internalDims.y);
}

void Renderer::handleRenderTargetsReset()
{
	const Int2 windowDims = this->window->getDimensions();
	const Int2 viewDims = this->window->getViewDimensions();
	const double resolutionScale = this->resolutionScaleFunc();
	const Int2 internalDims = MakeInternalRendererDimensions(viewDims, resolutionScale);
	this->backend->handleRenderTargetsReset(windowDims.x, windowDims.y, internalDims.x, internalDims.y);
}

VertexPositionBufferID Renderer::createVertexPositionBuffer(int vertexCount, int componentsPerVertex)
{
	const int bytesPerFloat = this->backend->getBytesPerFloat();
	return this->backend->createVertexPositionBuffer(vertexCount, componentsPerVertex, bytesPerFloat);
}

void Renderer::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	this->backend->freeVertexPositionBuffer(id);
}

bool Renderer::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	LockedBuffer lockedBuffer = this->backend->lockVertexPositionBuffer(id);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock vertex position buffer %d.", id);
		return false;
	}

	const int elementCount = positions.getCount();
	const int bytesPerFloat = this->backend->getBytesPerFloat();
	if (bytesPerFloat == sizeof(double))
	{
		Span<double> dstDoubles = lockedBuffer.getDoubles();
		DebugAssert(elementCount == dstDoubles.getCount());
		std::copy(positions.begin(), positions.end(), dstDoubles.begin());
	}
	else
	{
		Span<float> dstFloats = lockedBuffer.getFloats();
		DebugAssert(elementCount == dstFloats.getCount());
		std::transform(positions.begin(), positions.end(), dstFloats.begin(),
			[](double value)
		{
			return static_cast<float>(value);
		});
	}

	this->backend->unlockVertexPositionBuffer(id);
	return true;
}

VertexAttributeBufferID Renderer::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex)
{
	const int bytesPerFloat = this->backend->getBytesPerFloat();
	return this->backend->createVertexAttributeBuffer(vertexCount, componentsPerVertex, bytesPerFloat);
}

void Renderer::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	return this->backend->freeVertexAttributeBuffer(id);
}

bool Renderer::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	LockedBuffer lockedBuffer = this->backend->lockVertexAttributeBuffer(id);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock vertex attribute buffer %d.", id);
		return false;
	}

	const int elementCount = attributes.getCount();
	const int bytesPerFloat = this->backend->getBytesPerFloat();
	if (bytesPerFloat == sizeof(double))
	{
		Span<double> dstDoubles = lockedBuffer.getDoubles();
		DebugAssert(elementCount == dstDoubles.getCount());
		std::copy(attributes.begin(), attributes.end(), dstDoubles.begin());
	}
	else
	{
		Span<float> dstFloats = lockedBuffer.getFloats();
		DebugAssert(elementCount == dstFloats.getCount());
		std::transform(attributes.begin(), attributes.end(), dstFloats.begin(),
			[](double value)
		{
			return static_cast<float>(value);
		});
	}

	this->backend->unlockVertexAttributeBuffer(id);
	return true;
}

IndexBufferID Renderer::createIndexBuffer(int indexCount)
{
	constexpr int bytesPerIndex = sizeof(int32_t);
	return this->backend->createIndexBuffer(indexCount, bytesPerIndex);
}

void Renderer::freeIndexBuffer(IndexBufferID id)
{
	this->backend->freeIndexBuffer(id);
}

bool Renderer::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	LockedBuffer lockedBuffer = this->backend->lockIndexBuffer(id);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock index buffer %d.", id);
		return false;
	}

	Span<int32_t> dstIndices = lockedBuffer.getInts();

	DebugAssert(indices.getCount() == dstIndices.getCount());
	std::copy(indices.begin(), indices.end(), dstIndices.begin());
	this->backend->unlockIndexBuffer(id);
	return true;
}

UniformBufferID Renderer::createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement)
{
	return this->backend->createUniformBuffer(elementCount, bytesPerElement, alignmentOfElement);
}

UniformBufferID Renderer::createUniformBufferVector3s(int elementCount)
{
	static_assert(sizeof(Double3) == (sizeof(Float3) * 2));

	const int bytesPerFloat = this->backend->getBytesPerFloat();
	int bytesPerElement = sizeof(Double3);
	if (bytesPerFloat == 4)
	{
		bytesPerElement = sizeof(Float3);
	}

	const int alignmentOfElement = alignof(Double3);
	return this->createUniformBuffer(elementCount, bytesPerElement, alignmentOfElement);
}

UniformBufferID Renderer::createUniformBufferRenderTransforms(int elementCount)
{
	static_assert(sizeof(Matrix4d) == (sizeof(Matrix4f) * 2));
	static_assert(sizeof(RenderTransform) == (sizeof(Matrix4d) * 3));

	const int bytesPerFloat = this->backend->getBytesPerFloat();
	int bytesPerElement = sizeof(RenderTransform);
	if (bytesPerFloat == 4)
	{
		bytesPerElement /= 2;
	}

	const int alignmentOfElement = alignof(RenderTransform);
	return this->createUniformBuffer(elementCount, bytesPerElement, alignmentOfElement);
}

void Renderer::freeUniformBuffer(UniformBufferID id)
{
	this->backend->freeUniformBuffer(id);
}

bool Renderer::populateUniformBuffer(UniformBufferID id, Span<const std::byte> bytes)
{
	LockedBuffer lockedBuffer = this->backend->lockUniformBuffer(id);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock uniform buffer %d.", id);
		return false;
	}

	DebugAssert(bytes.getCount() == lockedBuffer.bytes.getCount());
	std::copy(bytes.begin(), bytes.end(), lockedBuffer.bytes.begin());
	this->backend->unlockUniformBuffer(id);
	return true;
}

bool Renderer::populateUniformBufferVector3s(UniformBufferID id, Span<const Double3> values)
{
	LockedBuffer lockedBuffer = this->backend->lockUniformBuffer(id);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock uniform buffer %d.", id);
		return false;
	}

	const int bytesPerFloat = this->backend->getBytesPerFloat();
	if (bytesPerFloat == sizeof(double))
	{
		Span<const std::byte> valueBytes(reinterpret_cast<const std::byte*>(values.begin()), values.getCount() * sizeof(Double3));
		DebugAssert(valueBytes.getCount() == lockedBuffer.bytes.getCount());
		std::copy(valueBytes.begin(), valueBytes.end(), lockedBuffer.bytes.begin());
	}
	else
	{
		for (int i = 0; i < values.getCount(); i++)
		{
			const Double3 &value = values[i];
			float *currentDstFloats = reinterpret_cast<float*>(lockedBuffer.bytes.begin()) + (i * 3);
			currentDstFloats[0] = static_cast<float>(value.x);
			currentDstFloats[1] = static_cast<float>(value.y);
			currentDstFloats[2] = static_cast<float>(value.z);
		}
	}

	this->backend->unlockUniformBuffer(id);
	return true;
}

bool Renderer::populateUniformBufferRenderTransforms(UniformBufferID id, Span<const RenderTransform> transforms)
{
	LockedBuffer lockedBuffer = this->backend->lockUniformBuffer(id);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock uniform buffer %d.", id);
		return false;
	}

	const int bytesPerFloat = this->backend->getBytesPerFloat();
	if (bytesPerFloat == sizeof(double))
	{
		Span<const std::byte> transformBytes(reinterpret_cast<const std::byte*>(transforms.begin()), transforms.getCount() * sizeof(RenderTransform));
		DebugAssert(transformBytes.getCount() == lockedBuffer.bytes.getCount());
		std::copy(transformBytes.begin(), transformBytes.end(), lockedBuffer.bytes.begin());
	}
	else
	{
		for (int i = 0; i < transforms.getCount(); i++)
		{
			const RenderTransform &transform = transforms[i];
			WriteRenderTransformFloat32(transform, lockedBuffer.bytes.begin() + (i * BytesPerTransformF));
		}
	}

	this->backend->unlockUniformBuffer(id);
	return true;
}

bool Renderer::populateUniformBufferIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformBytes)
{
	LockedBuffer lockedBuffer = this->backend->lockUniformBufferIndex(id, uniformIndex);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock uniform buffer %d index %d.", id, uniformIndex);
		return false;
	}

	DebugAssert(uniformBytes.getCount() == lockedBuffer.bytes.getCount());
	std::copy(uniformBytes.begin(), uniformBytes.end(), lockedBuffer.bytes.begin());
	this->backend->unlockUniformBufferIndex(id, uniformIndex);
	return true;
}

bool Renderer::populateUniformBufferIndexRenderTransform(UniformBufferID id, int uniformIndex, const RenderTransform &transform)
{
	LockedBuffer lockedBuffer = this->backend->lockUniformBufferIndex(id, uniformIndex);
	if (!lockedBuffer.isValid())
	{
		DebugLogErrorFormat("Couldn't lock uniform buffer %d at index %d.", id, uniformIndex);
		return false;
	}

	const int bytesPerFloat = this->backend->getBytesPerFloat();
	if (bytesPerFloat == sizeof(double))
	{
		Span<const std::byte> transformBytes(reinterpret_cast<const std::byte*>(&transform), sizeof(RenderTransform));
		DebugAssert(transformBytes.getCount() == lockedBuffer.bytes.getCount());
		std::copy(transformBytes.begin(), transformBytes.end(), lockedBuffer.bytes.begin());
	}
	else
	{
		WriteRenderTransformFloat32(transform, lockedBuffer.bytes.begin());
	}

	this->backend->unlockUniformBufferIndex(id, uniformIndex);
	return true;
}

RenderLightID Renderer::createLight()
{
	return this->backend->createLight();
}

void Renderer::freeLight(RenderLightID id)
{
	this->backend->freeLight(id);
}

bool Renderer::populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius)
{
	return this->backend->populateLight(id, point, startRadius, endRadius);
}

ObjectTextureID Renderer::createObjectTexture(int width, int height, int bytesPerTexel)
{
	return this->backend->createObjectTexture(width, height, bytesPerTexel);
}

void Renderer::freeObjectTexture(ObjectTextureID id)
{
	this->backend->freeObjectTexture(id);
}

std::optional<Int2> Renderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	return this->backend->tryGetObjectTextureDims(id);
}

LockedTexture Renderer::lockObjectTexture(ObjectTextureID id)
{
	return this->backend->lockObjectTexture(id);
}

void Renderer::unlockObjectTexture(ObjectTextureID id)
{
	this->backend->unlockObjectTexture(id);
}

bool Renderer::populateObjectTexture(ObjectTextureID id, Span<const std::byte> texels)
{
	LockedTexture lockedTexture = this->backend->lockObjectTexture(id);
	if (!lockedTexture.isValid())
	{
		DebugLogErrorFormat("Couldn't lock object texture %d.", id);
		return false;
	}

	DebugAssert(texels.getCount() == lockedTexture.texels.getCount());
	std::copy(texels.begin(), texels.end(), lockedTexture.texels.begin());

	this->backend->unlockObjectTexture(id);
	return true;
}

bool Renderer::populateObjectTexture8Bit(ObjectTextureID id, Span<const uint8_t> texels)
{
	Span<const std::byte> texelBytes(reinterpret_cast<const std::byte*>(texels.begin()), texels.getCount());
	return this->populateObjectTexture(id, texelBytes);
}

UiTextureID Renderer::createUiTexture(int width, int height)
{
	return this->backend->createUiTexture(width, height);
}

void Renderer::freeUiTexture(UiTextureID id)
{
	this->backend->freeUiTexture(id);
}

std::optional<Int2> Renderer::tryGetUiTextureDims(UiTextureID id) const
{
	return this->backend->tryGetUiTextureDims(id);
}

LockedTexture Renderer::lockUiTexture(UiTextureID id)
{
	return this->backend->lockUiTexture(id);
}

void Renderer::unlockUiTexture(UiTextureID id)
{
	this->backend->unlockUiTexture(id);
}

bool Renderer::populateUiTexture(UiTextureID id, Span<const std::byte> texels, const Palette *palette)
{
	LockedTexture lockedTexture = this->backend->lockUiTexture(id);
	if (!lockedTexture.isValid())
	{
		DebugLogErrorFormat("Couldn't lock UI texture %d.", id);
		return false;
	}

	Span2D<uint32_t> dstTexels = lockedTexture.getTexels32();

	if (palette == nullptr)
	{
		DebugAssert(texels.getCount() == lockedTexture.texels.getCount());
		Span<const uint32_t> srcTexels(reinterpret_cast<const uint32_t*>(texels.begin()), texels.getCount() / sizeof(uint32_t));
		std::copy(srcTexels.begin(), srcTexels.end(), dstTexels.begin());
	}
	else
	{
		DebugAssert(texels.getCount() == (lockedTexture.texels.getCount() / lockedTexture.bytesPerTexel));
		Span<const uint8_t> srcTexels(reinterpret_cast<const uint8_t*>(texels.begin()), texels.getCount());
		std::transform(srcTexels.begin(), srcTexels.end(), dstTexels.begin(),
			[palette](uint8_t texel)
		{
			return (*palette)[texel].toARGB();
		});
	}

	this->backend->unlockUiTexture(id);
	return true;
}

bool Renderer::populateUiTextureNoPalette(UiTextureID id, Span2D<const uint32_t> texels)
{
	Span<const std::byte> texelBytes(reinterpret_cast<const std::byte*>(texels.begin()), texels.getWidth() * texels.getHeight() * sizeof(uint32_t));
	return this->populateUiTexture(id, texelBytes);
}

/*void Renderer::drawPixel(const Color &color, int x, int y)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawPoint(this->renderer, x, y);
}

void Renderer::drawLine(const Color &color, int x1, int y1, int x2, int y2)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);
	SDL_RenderDrawLine(this->renderer, x1, y1, x2, y2);
}

void Renderer::drawRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderDrawRect(this->renderer, &rect);
}

void Renderer::fillRect(const Color &color, int x, int y, int w, int h)
{
	SDL_SetRenderDrawColor(this->renderer, color.r, color.g, color.b, color.a);

	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	SDL_RenderFillRect(this->renderer, &rect);
}*/

/*void Renderer::DrawLine(JPH::RVec3Arg src, JPH::RVec3Arg dst, JPH::ColorArg color)
{
	const RenderCamera &camera = g_physicsDebugCamera;
	const Double3 worldPoint0(static_cast<double>(src.GetX()), static_cast<double>(src.GetY()), static_cast<double>(src.GetZ()));
	const Double3 worldPoint1(static_cast<double>(dst.GetX()), static_cast<double>(dst.GetY()), static_cast<double>(dst.GetZ()));
	const double distSqr0 = (camera.worldPoint - worldPoint0).lengthSquared();
	const double distSqr1 = (camera.worldPoint - worldPoint1).lengthSquared();
	if ((distSqr0 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr1 > PHYSICS_DEBUG_MAX_DISTANCE_SQR))
	{
		return;
	}

	const Double4 clipPoint0 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint0, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint1 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint1, 1.0), camera.viewProjMatrix);
	if ((clipPoint0.w <= 0.0) || (clipPoint1.w <= 0.0))
	{
		return;
	}

	const Int2 viewDims = this->window->getViewDimensions();
	const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clipPoint0);
	const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clipPoint1);
	const Double2 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, viewDims.x, viewDims.y);
	const Double2 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, viewDims.x, viewDims.y);
	const Int2 pixelSpace0(static_cast<int>(screenSpace0.x), static_cast<int>(screenSpace0.y));
	const Int2 pixelSpace1(static_cast<int>(screenSpace1.x), static_cast<int>(screenSpace1.y));

	const double distanceRatio = std::max(distSqr0, distSqr1) / PHYSICS_DEBUG_MAX_DISTANCE_SQR;
	const double intensityPercent = std::clamp(1.0 - (distanceRatio * distanceRatio * distanceRatio), 0.0, 1.0);
	const ColorReal multipliedColor = ColorReal::fromARGB(color.GetUInt32()) * intensityPercent;
	const Color presentedColor = Color::fromARGB(multipliedColor.toARGB());
	this->drawLine(presentedColor, pixelSpace0.x, pixelSpace0.y, pixelSpace1.x, pixelSpace1.y);
}

void Renderer::DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow castShadow)
{
	const RenderCamera &camera = g_physicsDebugCamera;
	const Double3 worldPoint0(static_cast<double>(v1.GetX()), static_cast<double>(v1.GetY()), static_cast<double>(v1.GetZ()));
	const Double3 worldPoint1(static_cast<double>(v2.GetX()), static_cast<double>(v2.GetY()), static_cast<double>(v2.GetZ()));
	const Double3 worldPoint2(static_cast<double>(v3.GetX()), static_cast<double>(v3.GetY()), static_cast<double>(v3.GetZ()));
	const double distSqr0 = (camera.worldPoint - worldPoint0).lengthSquared();
	const double distSqr1 = (camera.worldPoint - worldPoint1).lengthSquared();
	const double distSqr2 = (camera.worldPoint - worldPoint2).lengthSquared();
	if ((distSqr0 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr1 > PHYSICS_DEBUG_MAX_DISTANCE_SQR) || (distSqr2 > PHYSICS_DEBUG_MAX_DISTANCE_SQR))
	{
		return;
	}

	const Double4 clipPoint0 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint0, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint1 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint1, 1.0), camera.viewProjMatrix);
	const Double4 clipPoint2 = RendererUtils::worldSpaceToClipSpace(Double4(worldPoint2, 1.0), camera.viewProjMatrix);
	if ((clipPoint0.w <= 0.0) || (clipPoint1.w <= 0.0) || (clipPoint2.w <= 0.0))
	{
		return;
	}

	const Int2 viewDims = this->window->getViewDimensions();
	const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clipPoint0);
	const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clipPoint1);
	const Double3 ndc2 = RendererUtils::clipSpaceToNDC(clipPoint2);
	const Double2 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, viewDims.x, viewDims.y);
	const Double2 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, viewDims.x, viewDims.y);
	const Double2 screenSpace2 = RendererUtils::ndcToScreenSpace(ndc2, viewDims.x, viewDims.y);
	const Double2 screenSpace01 = screenSpace1 - screenSpace0;
	const Double2 screenSpace12 = screenSpace2 - screenSpace1;
	const Double2 screenSpace20 = screenSpace0 - screenSpace2;
	const double screenSpace01Cross12 = screenSpace12.cross(screenSpace01);
	const double screenSpace12Cross20 = screenSpace20.cross(screenSpace12);
	const double screenSpace20Cross01 = screenSpace01.cross(screenSpace20);

	// Discard back-facing.
	const bool isFrontFacing = (screenSpace01Cross12 + screenSpace12Cross20 + screenSpace20Cross01) > 0.0;
	if (!isFrontFacing)
	{
		return;
	}

	this->DrawLine(v1, v2, color);
	this->DrawLine(v2, v3, color);
	this->DrawLine(v3, v1, color);
}

void Renderer::DrawText3D(JPH::RVec3Arg position, const std::string_view &str, JPH::ColorArg color, float height)
{
	// Do nothing.
}*/

void Renderer::submitFrame(const RenderCommandList &renderCommandList, const UiCommandList &uiCommandList,
	const RenderCamera &camera, const RenderFrameSettings &frameSettings)
{
	this->backend->submitFrame(renderCommandList, uiCommandList, camera, frameSettings);
}
