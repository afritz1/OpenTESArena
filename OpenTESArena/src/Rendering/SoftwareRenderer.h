#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstddef>
#include <cstdint>

#include "RenderLightUtils.h"
#include "RenderTextureAllocator.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"
#include "components/utilities/Buffer3D.h"
#include "components/utilities/RecyclablePool.h"
#include "components/utilities/Span.h"
#include "components/utilities/Span2D.h"
#include "components/utilities/Span3D.h"

struct Renderer3DProfilerData;

struct SoftwareObjectTexture
{
	Buffer<std::byte> texels;
	const uint8_t *texels8Bit;
	const uint32_t *texels32Bit;
	int width, height, texelCount;
	double widthReal, heightReal;
	int bytesPerTexel;

	SoftwareObjectTexture();

	void init(int width, int height, int bytesPerTexel);
	void clear();
};

struct SoftwareVertexPositionBuffer
{
	Buffer<double> positions;

	void init(int vertexCount, int componentsPerVertex);
};

struct SoftwareVertexAttributeBuffer
{
	Buffer<double> attributes;

	void init(int vertexCount, int componentsPerVertex);
};

struct SoftwareIndexBuffer
{
	Buffer<int32_t> indices;
	int triangleCount;

	void init(int indexCount);
};

struct SoftwareUniformBuffer
{
	Buffer<std::byte> bytes;
	int elementCount;
	int bytesPerElement;
	int alignmentOfElement;

	SoftwareUniformBuffer();

	void init(int elementCount, int bytesPerElement, int alignmentOfElement);

	std::byte *begin();
	const std::byte *begin() const;
	std::byte *end();
	const std::byte *end() const;

	template<typename T>
	T &get(int index)
	{
		DebugAssert(sizeof(T) == this->bytesPerElement);
		DebugAssert(alignof(T) == this->alignmentOfElement);
		DebugAssert(index >= 0);
		DebugAssert(index < this->elementCount);
		T *elementPtr = reinterpret_cast<T*>(this->begin());
		return elementPtr[index];
	}

	template<typename T>
	const T &get(int index) const
	{
		DebugAssert(sizeof(T) == this->bytesPerElement);
		DebugAssert(alignof(T) == this->alignmentOfElement);
		DebugAssert(index >= 0);
		DebugAssert(index < this->elementCount);
		const T *elementPtr = reinterpret_cast<const T*>(this->begin());
		return elementPtr[index];
	}

	// Potentially a subset of the bytes range due to padding/alignment.
	int getValidByteCount() const;
};

struct SoftwareLight
{
	double worldPointX;
	double worldPointY;
	double worldPointZ;
	double startRadius, startRadiusSqr;
	double endRadius, endRadiusSqr;
	double startEndRadiusDiff, startEndRadiusDiffRecip;

	SoftwareLight();

	void init(const Double3 &worldPoint, double startRadius, double endRadius);
};

using SoftwareVertexPositionBufferPool = RecyclablePool<VertexPositionBufferID, SoftwareVertexPositionBuffer>;
using SoftwareVertexAttributeBufferPool = RecyclablePool<VertexAttributeBufferID, SoftwareVertexAttributeBuffer>;
using SoftwareIndexBufferPool = RecyclablePool<IndexBufferID, SoftwareIndexBuffer>;
using SoftwareUniformBufferPool = RecyclablePool<UniformBufferID, SoftwareUniformBuffer>;
using SoftwareObjectTexturePool = RecyclablePool<ObjectTextureID, SoftwareObjectTexture>;
using SoftwareLightPool = RecyclablePool<RenderLightID, SoftwareLight>;

struct SoftwareObjectTextureAllocator final : public ObjectTextureAllocator
{
	SoftwareObjectTexturePool *pool;

	SoftwareObjectTextureAllocator();

	void init(SoftwareObjectTexturePool *pool);

	ObjectTextureID create(int width, int height, int bytesPerTexel) override;
	void free(ObjectTextureID textureID) override;

	std::optional<Int2> tryGetDimensions(ObjectTextureID textureID) const override;

	LockedTexture lock(ObjectTextureID textureID) override;
	void unlock(ObjectTextureID textureID) override;
};

class SoftwareRenderer
{
private:
	Buffer2D<uint8_t> paletteIndexBuffer; // Intermediate buffer to support back-to-front transparencies.
	Buffer2D<double> depthBuffer;
	Buffer3D<bool> ditherBuffer; // Stores N layers of pre-computed patterns depending on the option.
	DitheringMode ditheringMode;

	SoftwareVertexPositionBufferPool positionBuffers;
	SoftwareVertexAttributeBufferPool attributeBuffers;
	SoftwareIndexBufferPool indexBuffers;
	SoftwareUniformBufferPool uniformBuffers;
	SoftwareObjectTexturePool objectTextures;
	SoftwareLightPool lights;

	SoftwareObjectTextureAllocator textureAllocator;
public:
	SoftwareRenderer();
	~SoftwareRenderer();

	bool init(const RenderInitSettings &initSettings);
	void shutdown();
	bool isInited() const;

	void resize(int width, int height);

	Renderer3DProfilerData getProfilerData() const;

	int getBytesPerFloat() const;

	VertexPositionBufferID createVertexPositionBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent);
	void freeVertexPositionBuffer(VertexPositionBufferID id);
	LockedBuffer lockVertexPositionBuffer(VertexPositionBufferID id);
	void unlockVertexPositionBuffer(VertexPositionBufferID id);

	VertexAttributeBufferID createVertexAttributeBuffer(int vertexCount, int componentsPerVertex, int bytesPerComponent);
	void freeVertexAttributeBuffer(VertexAttributeBufferID id);
	LockedBuffer lockVertexAttributeBuffer(VertexAttributeBufferID id);
	void unlockVertexAttributeBuffer(VertexAttributeBufferID id);

	IndexBufferID createIndexBuffer(int indexCount, int bytesPerIndex);
	void freeIndexBuffer(IndexBufferID id);
	LockedBuffer lockIndexBuffer(IndexBufferID id);
	void unlockIndexBuffer(IndexBufferID id);

	ObjectTextureAllocator *getTextureAllocator();

	UniformBufferID createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement);
	void freeUniformBuffer(UniformBufferID id);
	LockedBuffer lockUniformBuffer(UniformBufferID id);
	LockedBuffer lockUniformBufferIndex(UniformBufferID id, int index);
	void unlockUniformBuffer(UniformBufferID id);
	void unlockUniformBufferIndex(UniformBufferID id, int index);

	RenderLightID createLight();
	void freeLight(RenderLightID id);
	bool populateLight(RenderLightID id, const Double3 &point, double startRadius, double endRadius);

	void submitFrame(const RenderCommandList &commandList, const RenderCamera &camera,
		const RenderFrameSettings &settings, uint32_t *outputBuffer);
};

#endif
