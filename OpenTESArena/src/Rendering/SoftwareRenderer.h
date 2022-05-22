#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "RendererSystem3D.h"
#include "RenderTriangle.h"
#include "../Math/MathUtils.h"
#include "../Math/Matrix4.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Math/Vector4.h"
#include "../Media/Palette.h"

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
		Buffer2D<uint8_t> texels;
		Buffer<uint32_t> paletteTexels;

		void init8Bit(int width, int height);
		void initPalette(int count);

		void clear();
	};

	using ObjectTexturePool = RecyclablePool<ObjectTexture, ObjectTextureID>;
	using ObjectMaterialPool = RecyclablePool<ObjectMaterial, ObjectMaterialID>;

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
private:
	using VertexBufferPool = RecyclablePool<VertexBuffer, VertexBufferID>;
	using AttributeBufferPool = RecyclablePool<AttributeBuffer, AttributeBufferID>;
	using IndexBufferPool = RecyclablePool<IndexBuffer, IndexBufferID>;

	Buffer2D<double> depthBuffer;
	VertexBufferPool vertexBuffers;
	AttributeBufferPool attributeBuffers;
	IndexBufferPool indexBuffers;
	ObjectTexturePool objectTextures;
	ObjectMaterialPool objectMaterials;
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

	bool tryCreateObjectTexture(int width, int height, bool isPalette, ObjectTextureID *outID) override;
	bool tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID) override;
	bool tryCreateObjectMaterial(ObjectTextureID id0, ObjectTextureID id1, ObjectMaterialID *outID) override;
	bool tryCreateObjectMaterial(ObjectTextureID id, ObjectMaterialID *outID) override;
	LockedTexture lockObjectTexture(ObjectTextureID id) override;
	void unlockObjectTexture(ObjectTextureID id) override;
	void freeObjectTexture(ObjectTextureID id) override;
	void freeObjectMaterial(ObjectMaterialID id) override;
	std::optional<Int2> tryGetObjectTextureDims(ObjectTextureID id) const override;
	bool tryGetObjectMaterialTextures(ObjectMaterialID id, ObjectTextureID *outID0, ObjectTextureID *outID1) const override;

	bool tryGetEntitySelectionData(const Double2 &uv, ObjectTextureID textureID, bool pixelPerfect,
		bool *outIsSelected) const override;

	Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
		Degrees fovY, double aspect) const override;

	ProfilerData getProfilerData() const override;

	void submitFrame(const RenderCamera &camera, const BufferView<const RenderDrawCall> &drawCalls,
		const RenderFrameSettings &settings, uint32_t *outputBuffer) override;
	void present() override;
};

#endif
