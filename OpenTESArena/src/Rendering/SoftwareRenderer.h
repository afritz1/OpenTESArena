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
private:
	Buffer2D<double> depthBuffer;
	ObjectTexturePool objectTextures;
	ObjectMaterialPool objectMaterials;
public:
	SoftwareRenderer();
	~SoftwareRenderer() override;

	void init(const RenderInitSettings &settings) override;
	void shutdown() override;
	bool isInited() const override;

	void resize(int width, int height) override;

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

	void submitFrame(const RenderCamera &camera, const BufferView<const RenderTriangle> &opaqueVoxelTriangles,
		const BufferView<const RenderTriangle> &alphaTestedVoxelTriangles, const BufferView<const RenderTriangle> &entityTriangles,
		const RenderFrameSettings &settings, uint32_t *outputBuffer) override;
	void present() override;
};

#endif
