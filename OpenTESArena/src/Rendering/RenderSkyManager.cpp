#include <vector>

#include "Renderer.h"
#include "RenderSkyManager.h"
#include "../Math/Constants.h"
#include "../Math/Vector3.h"
#include "../World/MeshUtils.h"

RenderSkyManager::RenderSkyManager()
{
	this->bgVertexBufferID = -1;
	this->bgNormalBufferID = -1;
	this->bgTexCoordBufferID = -1;
	this->bgIndexBufferID = -1;
	this->bgObjectTextureID = -1;
}

void RenderSkyManager::init(Renderer &renderer)
{
	std::vector<double> bgVertices;
	std::vector<double> bgNormals;
	std::vector<double> bgTexCoords;
	std::vector<int32_t> bgIndices;

	// Zenith and nadir.
	bgVertices.emplace_back(0.0);
	bgVertices.emplace_back(1.0);
	bgVertices.emplace_back(0.0);

	bgVertices.emplace_back(0.0);
	bgVertices.emplace_back(-1.0);
	bgVertices.emplace_back(0.0);

	const Double3 zenithPoint(bgVertices[0], bgVertices[1], bgVertices[2]);
	const Double3 nadirPoint(bgVertices[3], bgVertices[4], bgVertices[5]);
	const Double3 zenithNormal = -zenithPoint.normalized();
	const Double3 nadirNormal = -nadirPoint.normalized();
	bgNormals.emplace_back(zenithNormal.x);
	bgNormals.emplace_back(zenithNormal.y);
	bgNormals.emplace_back(zenithNormal.z);

	bgNormals.emplace_back(nadirNormal.x);
	bgNormals.emplace_back(nadirNormal.y);
	bgNormals.emplace_back(nadirNormal.z);

	bgTexCoords.emplace_back(0.50);
	bgTexCoords.emplace_back(0.0);

	bgTexCoords.emplace_back(0.50);
	bgTexCoords.emplace_back(0.0);

	constexpr int bgHorizonVertexCount = 8; // Arbitrary number of vertices.
	for (int i = 0; i <= bgHorizonVertexCount; i++)
	{
		const double percent = static_cast<double>(i) / static_cast<double>(bgHorizonVertexCount);
		const double period = percent * Constants::TwoPi;
		const double x = std::cos(period);
		const double z = std::sin(period);

		if (i < bgHorizonVertexCount)
		{
			// Add new horizon vertex.
			bgVertices.emplace_back(x);
			bgVertices.emplace_back(0.0);
			bgVertices.emplace_back(z);
		}

		if (i > 0)
		{
			// Generate vertex attributes + indices for two triangles. All normals should point at the player.
			const int bgVertexCount = static_cast<int>(bgVertices.size());
			const Double3 prevPoint(bgVertices[bgVertexCount - 6], bgVertices[bgVertexCount - 5], bgVertices[bgVertexCount - 4]);
			const Double3 curPoint(bgVertices[bgVertexCount - 3], bgVertices[bgVertexCount - 2], bgVertices[bgVertexCount - 1]);

			const Double3 prevNormal = -prevPoint.normalized();
			const Double3 curNormal = -curPoint.normalized();

			// Above-horizon winding: cur -> prev -> zenith
			bgNormals.emplace_back(curNormal.x);
			bgNormals.emplace_back(curNormal.y);
			bgNormals.emplace_back(curNormal.z);

			bgNormals.emplace_back(prevNormal.x);
			bgNormals.emplace_back(prevNormal.y);
			bgNormals.emplace_back(prevNormal.z);

			bgTexCoords.emplace_back(0.0);
			bgTexCoords.emplace_back(1.0);

			bgTexCoords.emplace_back(1.0);
			bgTexCoords.emplace_back(1.0);

			bgIndices.emplace_back(bgVertexCount - 1);
			bgIndices.emplace_back(bgVertexCount - 2);
			bgIndices.emplace_back(0);

			// Below-horizon winding: prev -> cur -> nadir
			bgNormals.emplace_back(prevNormal.x);
			bgNormals.emplace_back(prevNormal.y);
			bgNormals.emplace_back(prevNormal.z);

			bgNormals.emplace_back(curNormal.x);
			bgNormals.emplace_back(curNormal.y);
			bgNormals.emplace_back(curNormal.z);

			bgTexCoords.emplace_back(1.0);
			bgTexCoords.emplace_back(1.0);

			bgTexCoords.emplace_back(0.0);
			bgTexCoords.emplace_back(1.0);

			bgIndices.emplace_back(bgVertexCount - 2);
			bgIndices.emplace_back(bgVertexCount - 1);
			bgIndices.emplace_back(1);
		}
	}

	constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

	if (!renderer.tryCreateVertexBuffer(static_cast<int>(bgVertices.size()) / 3, positionComponentsPerVertex, &this->bgVertexBufferID))
	{
		DebugLogError("Couldn't create vertex buffer for sky background mesh ID.");
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(static_cast<int>(bgNormals.size()) / 3, normalComponentsPerVertex, &this->bgNormalBufferID))
	{
		DebugLogError("Couldn't create normal attribute buffer for sky background mesh ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(static_cast<int>(bgTexCoords.size()) / 2, texCoordComponentsPerVertex, &this->bgTexCoordBufferID))
	{
		DebugLogError("Couldn't create tex coord attribute buffer for sky background mesh ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateIndexBuffer(static_cast<int>(bgIndices.size()), &this->bgIndexBufferID))
	{
		DebugLogError("Couldn't create index buffer for sky background mesh ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	renderer.populateVertexBuffer(this->bgVertexBufferID, bgVertices);
	renderer.populateAttributeBuffer(this->bgNormalBufferID, bgNormals);
	renderer.populateAttributeBuffer(this->bgTexCoordBufferID, bgTexCoords);
	renderer.populateIndexBuffer(this->bgIndexBufferID, bgIndices);

	constexpr int bgTextureWidth = 1;
	constexpr int bgTextureHeight = 1;
	if (!renderer.tryCreateObjectTexture(bgTextureWidth, bgTextureHeight, 1, &this->bgObjectTextureID))
	{
		DebugLogError("Couldn't create object texture for sky background texture ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	LockedTexture bgLockedTexture = renderer.lockObjectTexture(this->bgObjectTextureID);
	uint8_t *bgTexels = static_cast<uint8_t*>(bgLockedTexture.texels);
	*bgTexels = 50; // @todo just a random palette index
	renderer.unlockObjectTexture(this->bgObjectTextureID);

	this->bgDrawCall.position = Double3::Zero;
	this->bgDrawCall.preScaleTranslation = Double3::Zero;
	this->bgDrawCall.rotation = Matrix4d::identity();
	this->bgDrawCall.scale = Matrix4d::identity();
	this->bgDrawCall.vertexBufferID = this->bgVertexBufferID;
	this->bgDrawCall.normalBufferID = this->bgNormalBufferID;
	this->bgDrawCall.texCoordBufferID = this->bgTexCoordBufferID;
	this->bgDrawCall.indexBufferID = this->bgIndexBufferID;
	this->bgDrawCall.textureIDs[0] = this->bgObjectTextureID;
	this->bgDrawCall.textureIDs[1] = std::nullopt;
	this->bgDrawCall.textureSamplingType0 = TextureSamplingType::Default;
	this->bgDrawCall.textureSamplingType1 = TextureSamplingType::Default;
	this->bgDrawCall.vertexShaderType = VertexShaderType::Voxel; // @todo: SkyBackground?
	this->bgDrawCall.pixelShaderType = PixelShaderType::Opaque; // @todo?
	this->bgDrawCall.pixelShaderParam0 = 0.0;
}

void RenderSkyManager::shutdown(Renderer &renderer)
{
	this->freeBgBuffers(renderer);
	this->bgDrawCall.clear();
}

void RenderSkyManager::freeBgBuffers(Renderer &renderer)
{
	if (this->bgVertexBufferID >= 0)
	{
		renderer.freeVertexBuffer(this->bgVertexBufferID);
		this->bgVertexBufferID = -1;
	}

	if (this->bgNormalBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->bgNormalBufferID);
		this->bgNormalBufferID = -1;
	}

	if (this->bgTexCoordBufferID >= 0)
	{
		renderer.freeAttributeBuffer(this->bgTexCoordBufferID);
		this->bgTexCoordBufferID = -1;
	}

	if (this->bgIndexBufferID >= 0)
	{
		renderer.freeIndexBuffer(this->bgIndexBufferID);
		this->bgIndexBufferID = -1;
	}

	if (this->bgObjectTextureID >= 0)
	{
		renderer.freeObjectTexture(this->bgObjectTextureID);
		this->bgObjectTextureID = -1;
	}
}

RenderDrawCall RenderSkyManager::getBgDrawCall() const
{
	return this->bgDrawCall;
}

void RenderSkyManager::update(const CoordDouble3 &cameraCoord)
{
	// Keep the sky centered on the player.
	this->bgDrawCall.position = VoxelUtils::coordToWorldPoint(cameraCoord);
}
