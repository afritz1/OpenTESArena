#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "RendererSystem3D.h"
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

class SoftwareRenderer : public RendererSystem3D
{
public:
	struct ObjectTexture
	{
		Buffer<std::byte> texels;
		const uint8_t *texels8Bit;
		const uint32_t *texels32Bit;
		int width, height, texelCount;
		int bytesPerTexel;

		ObjectTexture();

		void init(int width, int height, int bytesPerTexel);
		void clear();
	};

	using ObjectTexturePool = RecyclablePool<ObjectTexture, ObjectTextureID>;

	struct VertexBuffer
	{
		Buffer<double> vertices;

		void init(int vertexCount, int componentsPerVertex);
	};

	struct AttributeBuffer
	{
		Buffer<double> attributes;

		void init(int vertexCount, int componentsPerVertex);
	};

	struct IndexBuffer
	{
		Buffer<int32_t> indices;

		void init(int indexCount);
	};

	struct Light
	{
		Double3 worldPoint;
		double startRadius, endRadius;

		Light();

		void init(const Double3 &worldPoint, double startRadius, double endRadius);
	};
private:
	using VertexBufferPool = RecyclablePool<VertexBuffer, VertexBufferID>;
	using AttributeBufferPool = RecyclablePool<AttributeBuffer, AttributeBufferID>;
	using IndexBufferPool = RecyclablePool<IndexBuffer, IndexBufferID>;
	using LightPool = RecyclablePool<Light, RenderLightID>;

	Buffer2D<uint8_t> paletteIndexBuffer; // Intermediate buffer to support back-to-front transparencies.
	Buffer2D<double> depthBuffer;
	VertexBufferPool vertexBuffers;
	AttributeBufferPool attributeBuffers;
	IndexBufferPool indexBuffers;
	ObjectTexturePool objectTextures;
	LightPool lights;
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
	void populateVertexBuffer(VertexBufferID id, const BufferView<const double> &vertices) override;
	void populateAttributeBuffer(AttributeBufferID id, const BufferView<const double> &attributes) override;
	void populateIndexBuffer(IndexBufferID id, const BufferView<const int32_t> &indices) override;
	void freeVertexBuffer(VertexBufferID id) override;
	void freeAttributeBuffer(AttributeBufferID id) override;
	void freeIndexBuffer(IndexBufferID id) override;

	bool tryCreateObjectTexture(int width, int height, int bytesPerTexel, ObjectTextureID *outID) override;
	bool tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID) override;
	LockedTexture lockObjectTexture(ObjectTextureID id) override;
	void unlockObjectTexture(ObjectTextureID id) override;
	void freeObjectTexture(ObjectTextureID id) override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;

	bool tryCreateLight(RenderLightID *outID) override;
	void setLightPosition(RenderLightID id, const Double3 &worldPoint) override;
	void setLightRadius(RenderLightID id, double startRadius, double endRadius) override;
	void freeLight(RenderLightID id) override;

	ProfilerData getProfilerData() const override;

	void submitFrame(const RenderCamera &camera, const BufferView<const RenderDrawCall> &drawCalls,
		const RenderFrameSettings &settings, uint32_t *outputBuffer) override;
	void present() override;
};

#endif
