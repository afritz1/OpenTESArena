#include <numeric>
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

	const double pointDistance = 1000.0; // @todo: this is a hack while the sky is using naive depth testing w/o any occlusion culling, etc.

	constexpr int zenithVertexIndex = 0;
	constexpr int nadirVertexIndex = 1;
	const Double3 zenithPoint(0.0, 1.0 * pointDistance, 0.0);
	const Double3 nadirPoint(0.0, -1.0 * pointDistance, 0.0);
	bgVertices.emplace_back(zenithPoint.x);
	bgVertices.emplace_back(zenithPoint.y);
	bgVertices.emplace_back(zenithPoint.z);
	bgVertices.emplace_back(nadirPoint.x);
	bgVertices.emplace_back(nadirPoint.y);
	bgVertices.emplace_back(nadirPoint.z);

	const Double3 zenithNormal = -zenithPoint.normalized();
	const Double3 nadirNormal = -nadirPoint.normalized();
	bgNormals.emplace_back(zenithNormal.x);
	bgNormals.emplace_back(zenithNormal.y);
	bgNormals.emplace_back(zenithNormal.z);
	bgNormals.emplace_back(nadirNormal.x);
	bgNormals.emplace_back(nadirNormal.y);
	bgNormals.emplace_back(nadirNormal.z);

	const Double2 zenithTexCoord(0.50, 0.0);
	const Double2 nadirTexCoord(0.50, 1.0);
	bgTexCoords.emplace_back(zenithTexCoord.x);
	bgTexCoords.emplace_back(zenithTexCoord.y);
	bgTexCoords.emplace_back(nadirTexCoord.x);
	bgTexCoords.emplace_back(nadirTexCoord.y);

	constexpr int bgAboveHorizonTriangleCount = 16; // Arbitrary number of triangles, increases smoothness of cone shape.
	for (int i = 0; i < bgAboveHorizonTriangleCount; i++)
	{
		// Generate two triangles: one above horizon, one below.
		const double percent = static_cast<double>(i) / static_cast<double>(bgAboveHorizonTriangleCount);
		const double nextPercent = static_cast<double>(i + 1) / static_cast<double>(bgAboveHorizonTriangleCount);
		const double period = percent * Constants::TwoPi;
		const double nextPeriod = nextPercent * Constants::TwoPi;

		const Double3 point(std::cos(period) * pointDistance, 0.0, std::sin(period) * pointDistance);
		const Double3 nextPoint(std::cos(nextPeriod) * pointDistance, 0.0, std::sin(nextPeriod) * pointDistance);

		bgVertices.emplace_back(point.x);
		bgVertices.emplace_back(point.y);
		bgVertices.emplace_back(point.z);
		bgVertices.emplace_back(nextPoint.x);
		bgVertices.emplace_back(nextPoint.y);
		bgVertices.emplace_back(nextPoint.z);

		// Normals point toward the player.
		const Double3 normal = -point.normalized();
		const Double3 nextNormal = -nextPoint.normalized();
		bgNormals.emplace_back(normal.x);
		bgNormals.emplace_back(normal.y);
		bgNormals.emplace_back(normal.z);
		bgNormals.emplace_back(nextNormal.x);
		bgNormals.emplace_back(nextNormal.y);
		bgNormals.emplace_back(nextNormal.z);

		const Double2 texCoord(1.0, 1.0);
		const Double2 nextTexCoord(0.0, 1.0);
		bgTexCoords.emplace_back(texCoord.x);
		bgTexCoords.emplace_back(texCoord.y);
		bgTexCoords.emplace_back(nextTexCoord.x);
		bgTexCoords.emplace_back(nextTexCoord.y);

		// Above-horizon winding: next -> cur -> zenith
		const int32_t vertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 2);
		const int32_t nextVertexIndex = static_cast<int32_t>((bgVertices.size() / 3) - 1);
		bgIndices.emplace_back(nextVertexIndex);
		bgIndices.emplace_back(vertexIndex);
		bgIndices.emplace_back(zenithVertexIndex);

		// Below-horizon winding: cur -> next -> nadir
		bgIndices.emplace_back(vertexIndex);
		bgIndices.emplace_back(nextVertexIndex);
		bgIndices.emplace_back(nadirVertexIndex);
	}

	constexpr int positionComponentsPerVertex = MeshUtils::POSITION_COMPONENTS_PER_VERTEX;
	constexpr int normalComponentsPerVertex = MeshUtils::NORMAL_COMPONENTS_PER_VERTEX;
	constexpr int texCoordComponentsPerVertex = MeshUtils::TEX_COORDS_PER_VERTEX;

	const int bgVertexCount = static_cast<int>(bgVertices.size()) / 3;
	if (!renderer.tryCreateVertexBuffer(bgVertexCount, positionComponentsPerVertex, &this->bgVertexBufferID))
	{
		DebugLogError("Couldn't create vertex buffer for sky background mesh ID.");
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(bgVertexCount, normalComponentsPerVertex, &this->bgNormalBufferID))
	{
		DebugLogError("Couldn't create normal attribute buffer for sky background mesh ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	if (!renderer.tryCreateAttributeBuffer(bgVertexCount, texCoordComponentsPerVertex, &this->bgTexCoordBufferID))
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
	constexpr int bgTextureHeight = 2; // @todo: figure out sky background texture coloring; probably lock+update the main world palette in an update() with DAYTIME.COL indices as times goes on?
	constexpr int bgBytesPerTexel = 1;
	if (!renderer.tryCreateObjectTexture(bgTextureWidth, bgTextureHeight, bgBytesPerTexel, &this->bgObjectTextureID))
	{
		DebugLogError("Couldn't create object texture for sky background texture ID.");
		this->freeBgBuffers(renderer);
		return;
	}

	LockedTexture bgLockedTexture = renderer.lockObjectTexture(this->bgObjectTextureID);
	uint8_t *bgTexels = static_cast<uint8_t*>(bgLockedTexture.texels);
	std::iota(bgTexels, bgTexels + (bgTextureWidth * bgTextureHeight), 126); // @todo: figure out which palette indices are used for the sky
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
