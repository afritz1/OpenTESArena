#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <cstddef>
#include <cstdint>

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
#include "components/utilities/KeyValuePool.h"
#include "components/utilities/Span.h"
#include "components/utilities/Span2D.h"
#include "components/utilities/Span3D.h"

struct RendererProfilerData3D;

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

struct SoftwareMaterial
{
	VertexShaderType vertexShaderType;
	FragmentShaderType fragmentShaderType;

	ObjectTextureID textureIDs[RenderMaterialKey::MAX_TEXTURE_COUNT];
	int textureCount;

	RenderLightingType lightingType;

	bool enableBackFaceCulling;
	bool enableDepthRead;
	bool enableDepthWrite;

	SoftwareMaterial();

	void init(VertexShaderType vertexShaderType, FragmentShaderType fragmentShaderType, Span<const ObjectTextureID> textureIDs,
		RenderLightingType lightingType, bool enableBackFaceCulling, bool enableDepthRead, bool enableDepthWrite);
};

struct SoftwareMaterialInstance
{
	double meshLightPercent;
	double texCoordAnimPercent;

	SoftwareMaterialInstance();
};

struct SoftwareLight
{
	double pointX;
	double pointY;
	double pointZ;
	double startRadius, startRadiusSqr;
	double endRadius, endRadiusSqr;
	double radiusDiffRecip;

	SoftwareLight();

	void init(const Double3 &point, double startRadius, double endRadius);
};

using SoftwareVertexPositionBufferPool = KeyValuePool<VertexPositionBufferID, SoftwareVertexPositionBuffer>;
using SoftwareVertexAttributeBufferPool = KeyValuePool<VertexAttributeBufferID, SoftwareVertexAttributeBuffer>;
using SoftwareIndexBufferPool = KeyValuePool<IndexBufferID, SoftwareIndexBuffer>;
using SoftwareUniformBufferPool = KeyValuePool<UniformBufferID, SoftwareUniformBuffer>;
using SoftwareObjectTexturePool = KeyValuePool<ObjectTextureID, SoftwareObjectTexture>;
using SoftwareMaterialPool = KeyValuePool<RenderMaterialID, SoftwareMaterial>;
using SoftwareMaterialInstancePool = KeyValuePool<RenderMaterialInstanceID, SoftwareMaterialInstance>;

class SoftwareRenderer
{
private:
	Buffer2D<uint8_t> paletteIndexBuffer; // Intermediate buffer to support back-to-front transparencies.
	Buffer2D<double> depthBuffer;

	SoftwareVertexPositionBufferPool positionBuffers;
	SoftwareVertexAttributeBufferPool attributeBuffers;
	SoftwareIndexBufferPool indexBuffers;
	SoftwareUniformBufferPool uniformBuffers;
	SoftwareObjectTexturePool objectTextures;
	SoftwareMaterialPool materials;
	SoftwareMaterialInstancePool materialInsts;
public:
	SoftwareRenderer();
	~SoftwareRenderer();

	bool init(const RenderInitSettings &initSettings);
	void shutdown();
	bool isInited() const;

	void resize(int width, int height);

	RendererProfilerData3D getProfilerData() const;

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

	UniformBufferID createUniformBuffer(int elementCount, int bytesPerElement, int alignmentOfElement);
	void freeUniformBuffer(UniformBufferID id);
	LockedBuffer lockUniformBuffer(UniformBufferID id);
	LockedBuffer lockUniformBufferIndex(UniformBufferID id, int index);
	void unlockUniformBuffer(UniformBufferID id);
	void unlockUniformBufferIndex(UniformBufferID id, int index);

	ObjectTextureID createTexture(int width, int height, int bytesPerTexel);
	void freeTexture(ObjectTextureID id);
	std::optional<Int2> tryGetTextureDims(ObjectTextureID id) const;
	LockedTexture lockTexture(ObjectTextureID id);
	void unlockTexture(ObjectTextureID id);

	RenderMaterialID createMaterial(RenderMaterialKey key);
	void freeMaterial(RenderMaterialID id);
	
	RenderMaterialInstanceID createMaterialInstance();
	void freeMaterialInstance(RenderMaterialInstanceID id);
	void setMaterialInstanceMeshLightPercent(RenderMaterialInstanceID id, double value);
	void setMaterialInstanceTexCoordAnimPercent(RenderMaterialInstanceID id, double value);

	void submitFrame(const RenderCommandList &commandList, const RenderCamera &camera,
		const RenderFrameSettings &settings, uint32_t *outputBuffer);
};

#endif
