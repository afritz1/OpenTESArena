#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstddef>
#include <cstdint>

#include "RendererSystem3D.h"
#include "RenderLightUtils.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Utilities/Palette.h"

#include "components/utilities/Buffer.h"
#include "components/utilities/Buffer2D.h"
#include "components/utilities/Buffer3D.h"
#include "components/utilities/BufferView.h"
#include "components/utilities/BufferView2D.h"
#include "components/utilities/BufferView3D.h"
#include "components/utilities/RecyclablePool.h"

struct RasterizerBin; // For triangle binning.

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

struct SoftwareVertexBuffer
{
	Buffer<double> vertices;

	void init(int vertexCount, int componentsPerVertex);
};

struct SoftwareAttributeBuffer
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
	size_t sizeOfElement;
	size_t alignmentOfElement;

	SoftwareUniformBuffer();

	void init(int elementCount, size_t sizeOfElement, size_t alignmentOfElement);

	std::byte *begin();
	const std::byte *begin() const;
	std::byte *end();
	const std::byte *end() const;

	template<typename T>
	T &get(int index)
	{
		DebugAssert(sizeof(T) == this->sizeOfElement);
		DebugAssert(alignof(T) == this->alignmentOfElement);
		DebugAssert(index >= 0);
		DebugAssert(index < this->elementCount);
		T *elementPtr = reinterpret_cast<T*>(this->begin());
		return elementPtr[index];
	}

	template<typename T>
	const T &get(int index) const
	{
		DebugAssert(sizeof(T) == this->sizeOfElement);
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

using SoftwareVertexBufferPool = RecyclablePool<SoftwareVertexBuffer, VertexBufferID>;
using SoftwareAttributeBufferPool = RecyclablePool<SoftwareAttributeBuffer, AttributeBufferID>;
using SoftwareIndexBufferPool = RecyclablePool<SoftwareIndexBuffer, IndexBufferID>;
using SoftwareUniformBufferPool = RecyclablePool<SoftwareUniformBuffer, UniformBufferID>;
using SoftwareObjectTexturePool = RecyclablePool<SoftwareObjectTexture, ObjectTextureID>;
using SoftwareLightPool = RecyclablePool<SoftwareLight, RenderLightID>;

class SoftwareRenderer : public RendererSystem3D
{
private:
	Buffer2D<uint8_t> paletteIndexBuffer; // Intermediate buffer to support back-to-front transparencies.
	Buffer2D<double> depthBuffer;
	Buffer3D<bool> ditherBuffer; // Stores N layers of pre-computed patterns depending on the option.
	DitheringMode ditheringMode;

	SoftwareVertexBufferPool vertexBuffers;
	SoftwareAttributeBufferPool attributeBuffers;
	SoftwareIndexBufferPool indexBuffers;
	SoftwareUniformBufferPool uniformBuffers;
	SoftwareObjectTexturePool objectTextures;
	SoftwareLightPool lights;
public:
	SoftwareRenderer();
	~SoftwareRenderer() override;

	void init(const RenderInitSettings &settings) override;
	void shutdown() override;
	bool isInited() const override;

	void resize(int width, int height) override;

	bool tryCreateVertexBuffer(int vertexCount, int componentsPerVertex, VertexBufferID *outID) override;
	bool tryCreateAttributeBuffer(int vertexCount, int componentsPerVertex, AttributeBufferID *outID) override;
	bool tryCreateIndexBuffer(int indexCount, IndexBufferID *outID) override;
	void populateVertexBuffer(VertexBufferID id, BufferView<const double> vertices) override;
	void populateAttributeBuffer(AttributeBufferID id, BufferView<const double> attributes) override;
	void populateIndexBuffer(IndexBufferID id, BufferView<const int32_t> indices) override;
	void freeVertexBuffer(VertexBufferID id) override;
	void freeAttributeBuffer(AttributeBufferID id) override;
	void freeIndexBuffer(IndexBufferID id) override;

	bool tryCreateObjectTexture(int width, int height, int bytesPerTexel, ObjectTextureID *outID) override;
	bool tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID) override;
	LockedTexture lockObjectTexture(ObjectTextureID id) override;
	void unlockObjectTexture(ObjectTextureID id) override;
	void freeObjectTexture(ObjectTextureID id) override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;

	bool tryCreateUniformBuffer(int elementCount, size_t sizeOfElement, size_t alignmentOfElement, UniformBufferID *outID) override;
	void populateUniformBuffer(UniformBufferID id, BufferView<const std::byte> data) override;
	void populateUniformAtIndex(UniformBufferID id, int uniformIndex, BufferView<const std::byte> uniformData) override;
	void freeUniformBuffer(UniformBufferID id) override;

	bool tryCreateLight(RenderLightID *outID) override;
	void setLightPosition(RenderLightID id, const Double3 &worldPoint) override;
	void setLightRadius(RenderLightID id, double startRadius, double endRadius) override;
	void freeLight(RenderLightID id) override;

	Renderer3DProfilerData getProfilerData() const override;

	void submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings,
		const RenderCommandBuffer &commandBuffer, uint32_t *outputBuffer) override;
	void present() override;
};

#endif
