#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "RenderBackendType.h"
#include "RenderCamera.h"
#include "Renderer.h"
#include "RendererUtils.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
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
}

RenderElement2D::RenderElement2D(UiTextureID id, double x, double y, double width, double height)
{
	this->id = id;
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;
}

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
	}

	this->window = nullptr;
}

bool Renderer::init(const Window *window, RenderBackendType backendType, const RenderResolutionScaleFunc &resolutionScaleFunc,
	int renderThreadsMode, DitheringMode ditheringMode, const std::string &dataFolderPath)
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
	initSettings.init(window, dataFolderPath, internalRenderDims.x, internalRenderDims.y, renderThreadsMode, ditheringMode);
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
	return this->backend->createVertexPositionBuffer(vertexCount, componentsPerVertex);
}

VertexAttributeBufferID Renderer::createVertexAttributeBuffer(int vertexCount, int componentsPerVertex)
{
	return this->backend->createVertexAttributeBuffer(vertexCount, componentsPerVertex);
}

IndexBufferID Renderer::createIndexBuffer(int indexCount)
{
	return this->backend->createIndexBuffer(indexCount);
}

void Renderer::populateVertexPositionBuffer(VertexPositionBufferID id, Span<const double> positions)
{
	this->backend->populateVertexPositionBuffer(id, positions);
}

void Renderer::populateVertexAttributeBuffer(VertexAttributeBufferID id, Span<const double> attributes)
{
	this->backend->populateVertexAttributeBuffer(id, attributes);
}

void Renderer::populateIndexBuffer(IndexBufferID id, Span<const int32_t> indices)
{
	this->backend->populateIndexBuffer(id, indices);
}

void Renderer::freeVertexPositionBuffer(VertexPositionBufferID id)
{
	this->backend->freeVertexPositionBuffer(id);
}

void Renderer::freeVertexAttributeBuffer(VertexAttributeBufferID id)
{
	return this->backend->freeVertexAttributeBuffer(id);
}

void Renderer::freeIndexBuffer(IndexBufferID id)
{
	this->backend->freeIndexBuffer(id);
}

ObjectTextureID Renderer::createObjectTexture(int width, int height, int bytesPerTexel)
{
	ObjectTextureAllocator *allocator = this->backend->getObjectTextureAllocator();
	return allocator->create(width, height, bytesPerTexel);
}

ObjectTextureID Renderer::createObjectTexture(const TextureBuilder &textureBuilder)
{
	ObjectTextureAllocator *allocator = this->backend->getObjectTextureAllocator();
	return allocator->create(textureBuilder);
}

UiTextureID Renderer::createUiTexture(int width, int height)
{
	UiTextureAllocator *allocator = this->backend->getUiTextureAllocator();
	return allocator->create(width, height);
}

UiTextureID Renderer::createUiTexture(Span2D<const uint32_t> texels)
{
	UiTextureAllocator *allocator = this->backend->getUiTextureAllocator();
	return allocator->create(texels);
}

UiTextureID Renderer::createUiTexture(Span2D<const uint8_t> texels, const Palette &palette)
{
	UiTextureAllocator *allocator = this->backend->getUiTextureAllocator();
	return allocator->create(texels, palette);
}

UiTextureID Renderer::createUiTexture(TextureBuilderID textureBuilderID, PaletteID paletteID, const TextureManager &textureManager)
{
	UiTextureAllocator *allocator = this->backend->getUiTextureAllocator();
	return allocator->create(textureBuilderID, paletteID, textureManager);
}

std::optional<Int2> Renderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	return this->backend->tryGetObjectTextureDims(id);
}

std::optional<Int2> Renderer::tryGetUiTextureDims(UiTextureID id) const
{
	return this->backend->tryGetUiTextureDims(id);
}

LockedTexture Renderer::lockObjectTexture(ObjectTextureID id)
{
	ObjectTextureAllocator *allocator = this->backend->getObjectTextureAllocator();
	return allocator->lock(id);
}

LockedTexture Renderer::lockUiTexture(UiTextureID textureID)
{
	UiTextureAllocator *allocator = this->backend->getUiTextureAllocator();
	return allocator->lock(textureID);
}

void Renderer::unlockObjectTexture(ObjectTextureID id)
{
	ObjectTextureAllocator *allocator = this->backend->getObjectTextureAllocator();
	allocator->unlock(id);
}

void Renderer::unlockUiTexture(UiTextureID textureID)
{
	UiTextureAllocator *allocator = this->backend->getUiTextureAllocator();
	allocator->unlock(textureID);
}

void Renderer::freeObjectTexture(ObjectTextureID id)
{
	ObjectTextureAllocator *allocator = this->backend->getObjectTextureAllocator();
	allocator->free(id);
}

void Renderer::freeUiTexture(UiTextureID id)
{
	UiTextureAllocator *allocator = this->backend->getUiTextureAllocator();
	allocator->free(id);
}

UniformBufferID Renderer::createUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement)
{
	return this->backend->createUniformBuffer(elementCount, sizeOfElement, alignmentOfElement);
}

void Renderer::populateUniformBuffer(UniformBufferID id, Span<const std::byte> data)
{
	this->backend->populateUniformBuffer(id, data);
}

void Renderer::populateUniformAtIndex(UniformBufferID id, int uniformIndex, Span<const std::byte> uniformData)
{
	this->backend->populateUniformAtIndex(id, uniformIndex, uniformData);
}

void Renderer::freeUniformBuffer(UniformBufferID id)
{
	this->backend->freeUniformBuffer(id);
}

RenderLightID Renderer::createLight()
{
	return this->backend->createLight();
}

void Renderer::setLightPosition(RenderLightID id, const Double3 &worldPoint)
{
	this->backend->setLightPosition(id, worldPoint);
}

void Renderer::setLightRadius(RenderLightID id, double startRadius, double endRadius)
{
	this->backend->setLightRadius(id, startRadius, endRadius);
}

void Renderer::freeLight(RenderLightID id)
{
	this->backend->freeLight(id);
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
