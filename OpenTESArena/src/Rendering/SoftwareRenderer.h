#ifndef SOFTWARE_RENDERER_H
#define SOFTWARE_RENDERER_H

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "RendererSystem3D.h"
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

class SoftwareRenderer : public RendererSystem3D
{
private:
	struct ObjectTexture
	{
		Buffer2D<uint8_t> texels;

		void init(int width, int height);
	};

	Buffer2D<double> depthBuffer;

	std::vector<ObjectTexture> objectTextures;
	std::vector<ObjectTextureID> freedObjectTextureIDs;
	ObjectTextureID nextObjectTextureID;

	ObjectTextureID getNextObjectTextureID();
public:
	SoftwareRenderer();
	~SoftwareRenderer() override;

	void init(const RenderInitSettings &settings) override;
	void shutdown() override;
	bool isInited() const override;

	void resize(int width, int height) override;

	bool tryCreateObjectTexture(int width, int height, ObjectTextureID *outID) override;
	bool tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID) override;
	LockedTexture lockObjectTexture(ObjectTextureID id) override;
	void unlockObjectTexture(ObjectTextureID id) override;
	void freeObjectTexture(ObjectTextureID id) override;

	bool tryGetEntitySelectionData(const Double2 &uv, ObjectTextureID textureID, bool pixelPerfect,
		bool *outIsSelected) const override;

	Double3 screenPointToRay(double xPercent, double yPercent, const Double3 &cameraDirection,
		Degrees fovY, double aspect) const override;

	ProfilerData getProfilerData() const override;

	void submitFrame(const RenderCamera &camera, const RenderFrameSettings &settings, uint32_t *outputBuffer) override;
	void present() override;
};

#endif
