#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>

#include "ArenaRenderUtils.h"
#include "LegacyRendererUtils.h"
#include "RenderCamera.h"
#include "RenderDrawCall.h"
#include "RendererUtils.h"
#include "RenderFrameSettings.h"
#include "RenderInitSettings.h"
#include "SoftwareRenderer.h"
#include "../Assets/TextureBuilder.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Math/Random.h"
#include "../Utilities/Color.h"
#include "../Utilities/Palette.h"
#include "../World/ChunkUtils.h"

#include "components/debug/Debug.h"

// Internal geometry types/functions.
namespace swGeometry
{
	struct ClippingPlane
	{
		Double3 point;
		Double3 normal;
	};

	using ClippingPlanes = std::array<ClippingPlane, 5>;

	// Plane point and normal pairs in world space.
	ClippingPlanes MakeClippingPlanes(const RenderCamera &camera)
	{
		const ClippingPlanes planes =
		{
			{
				// Near plane (far plane is not necessary due to how chunks are managed - it only matters if a view distance slider exists)
				{ camera.worldPoint + (camera.forward * RendererUtils::NEAR_PLANE), camera.forward },
				// Left
				{ camera.worldPoint, camera.leftFrustumNormal },
				// Right
				{ camera.worldPoint, camera.rightFrustumNormal },
				// Bottom
				{ camera.worldPoint, camera.bottomFrustumNormal },
				// Top
				{ camera.worldPoint, camera.topFrustumNormal }
			}
		};

		return planes;
	}

	struct TriangleClipResult
	{
		static constexpr int MAX_RESULTS = 2;

		int triangleCount = 0;
		Double3 v0s[MAX_RESULTS], v1s[MAX_RESULTS], v2s[MAX_RESULTS];
		Double3 v0v1s[MAX_RESULTS], v1v2s[MAX_RESULTS], v2v0s[MAX_RESULTS];
		Double3 normal0s[MAX_RESULTS], normal1s[MAX_RESULTS], normal2s[MAX_RESULTS];
		Double2 uv0s[MAX_RESULTS], uv1s[MAX_RESULTS], uv2s[MAX_RESULTS];
	private:
		void populateIndex(int index, const Double3 &v0, const Double3 &v1, const Double3 &v2,
			const Double3 &normal0, const Double3 &normal1, const Double3 &normal2,
			const Double2 &uv0, const Double2 &uv1, const Double2 &uv2)
		{
			this->v0s[index] = v0;
			this->v1s[index] = v1;
			this->v2s[index] = v2;
			this->v0v1s[index] = v1 - v0;
			this->v1v2s[index] = v2 - v1;
			this->v2v0s[index] = v0 - v2;
			this->normal0s[index] = normal0;
			this->normal1s[index] = normal1;
			this->normal2s[index] = normal2;
			this->uv0s[index] = uv0;
			this->uv1s[index] = uv1;
			this->uv2s[index] = uv2;
		}
	public:
		static TriangleClipResult zero()
		{
			TriangleClipResult result;
			result.triangleCount = 0;
			return result;
		}

		static TriangleClipResult one(const Double3 &v0, const Double3 &v1, const Double3 &v2,
			const Double3 &normal0, const Double3 &normal1, const Double3 &normal2,
			const Double2 &uv0, const Double2 &uv1, const Double2 &uv2)
		{
			TriangleClipResult result;
			result.triangleCount = 1;
			result.populateIndex(0, v0, v1, v2, normal0, normal1, normal2, uv0, uv1, uv2);
			return result;
		}

		static TriangleClipResult two(const Double3 &v0A, const Double3 &v1A, const Double3 &v2A,
			const Double3 &normal0A, const Double3 &normal1A, const Double3 &normal2A,
			const Double2 &uv0A, const Double2 &uv1A, const Double2 &uv2A,
			const Double3 &v0B, const Double3 &v1B, const Double3 &v2B,
			const Double3 &normal0B, const Double3 &normal1B, const Double3 &normal2B,
			const Double2 &uv0B, const Double2 &uv1B, const Double2 &uv2B)
		{
			TriangleClipResult result;
			result.triangleCount = 2;
			result.populateIndex(0, v0A, v1A, v2A, normal0A, normal1A, normal2A, uv0A, uv1A, uv2A);
			result.populateIndex(1, v0B, v1B, v2B, normal0B, normal1B, normal2B, uv0B, uv1B, uv2B);
			return result;
		}
	};

	struct TriangleDrawListIndices
	{
		int startIndex;
		int count;

		TriangleDrawListIndices(int startIndex, int count)
		{
			this->startIndex = startIndex;
			this->count = count;
		}
	};

	TriangleClipResult ClipTriangle(const Double3 &v0, const Double3 &v1, const Double3 &v2,
		const Double3 &normal0, const Double3 &normal1, const Double3 &normal2,
		const Double2 &uv0, const Double2 &uv1, const Double2 &uv2, const Double3 &eye,
		const Double3 &planePoint, const Double3 &planeNormal)
	{
		std::array<const Double3*, 3> insidePoints, outsidePoints;
		std::array<const Double3*, 3> insideNormals, outsideNormals;
		std::array<const Double2*, 3> insideUVs, outsideUVs;
		int insidePointCount = 0;
		int outsidePointCount = 0;

		const std::array<const Double3*, 3> vertexPtrs = { &v0, &v1, &v2 };
		const std::array<const Double3*, 3> normalPtrs = { &normal0, &normal1, &normal2 };
		const std::array<const Double2*, 3> uvPtrs = { &uv0, &uv1, &uv2 };

		// Determine which vertices are in the positive half-space of the clipping plane.
		for (int i = 0; i < static_cast<int>(vertexPtrs.size()); i++)
		{
			const Double3 *vertexPtr = vertexPtrs[i];
			const Double3 *normalPtr = normalPtrs[i];
			const double dist = MathUtils::distanceToPlane(*vertexPtr, planePoint, planeNormal);
			if (dist >= 0.0)
			{
				insidePoints[insidePointCount] = vertexPtr;
				insideNormals[insidePointCount] = normalPtr;
				insideUVs[insidePointCount] = uvPtrs[i];
				insidePointCount++;
			}
			else
			{
				outsidePoints[outsidePointCount] = vertexPtr;
				outsideNormals[outsidePointCount] = normalPtr;
				outsideUVs[outsidePointCount] = uvPtrs[i];
				outsidePointCount++;
			}
		}

		// Clip triangle depending on the inside/outside vertex case.
		const bool isCompletelyOutside = insidePointCount == 0;
		const bool isCompletelyInside = insidePointCount == 3;
		const bool becomesSmallerTriangle = insidePointCount == 1;
		const bool becomesQuad = insidePointCount == 2;
		if (isCompletelyOutside)
		{
			return TriangleClipResult::zero();
		}
		else if (isCompletelyInside)
		{
			// Reverse vertex order if back-facing.
			if ((eye - v0).dot(normal0) >= Constants::Epsilon)
			{
				return TriangleClipResult::one(v0, v1, v2, normal0, normal1, normal2, uv0, uv1, uv2);
			}
			else
			{
				return TriangleClipResult::one(v0, v2, v1, normal0, normal2, normal1, uv0, uv2, uv1);
			}
		}
		else if (becomesSmallerTriangle)
		{
			const Double3 &insidePoint = *insidePoints[0];
			const Double3 &insideNormal = *insideNormals[0];
			const Double2 &insideUV = *insideUVs[0];
			const Double3 &outsidePoint0 = *outsidePoints[0];
			const Double3 &outsidePoint1 = *outsidePoints[1];

			// @todo: replace ray-plane intersection with one that get T value internally
			Double3 newInsidePoint1, newInsidePoint2;
			MathUtils::rayPlaneIntersection(insidePoint, (outsidePoint0 - insidePoint).normalized(),
				planePoint, planeNormal, &newInsidePoint1);
			MathUtils::rayPlaneIntersection(insidePoint, (outsidePoint1 - insidePoint).normalized(),
				planePoint, planeNormal, &newInsidePoint2);

			const double t0 = (outsidePoint0 - insidePoint).length();
			const double t1 = (outsidePoint1 - insidePoint).length();
			const double newT0 = (newInsidePoint1 - insidePoint).length();
			const double newT1 = (newInsidePoint2 - insidePoint).length();

			const Double3 outsideNormal0 = *outsideNormals[0];
			const Double3 outsideNormal1 = *outsideNormals[1];
			const Double3 newInsideNormal0 = insideNormal.lerp(outsideNormal0, newT0 / t0);
			const Double3 newInsideNormal1 = insideNormal.lerp(outsideNormal1, newT1 / t1);

			const Double2 outsideUV0 = *outsideUVs[0];
			const Double2 outsideUV1 = *outsideUVs[1];
			const Double2 newInsideUV0 = insideUV.lerp(outsideUV0, newT0 / t0);
			const Double2 newInsideUV1 = insideUV.lerp(outsideUV1, newT1 / t1);

			// Swap vertex winding if needed so we don't generate a back-facing triangle from a front-facing one.
			const Double3 unormal = (insidePoint - newInsidePoint2).cross(newInsidePoint1 - insidePoint);
			if ((eye - insidePoint).dot(unormal) >= Constants::Epsilon)
			{
				return TriangleClipResult::one(insidePoint, newInsidePoint1, newInsidePoint2, insideNormal,
					newInsideNormal0, newInsideNormal1, insideUV, newInsideUV0, newInsideUV1);
			}
			else
			{
				return TriangleClipResult::one(newInsidePoint2, newInsidePoint1, insidePoint, newInsideNormal1,
					newInsideNormal0, insideNormal, newInsideUV1, newInsideUV0, insideUV);
			}
		}
		else if (becomesQuad)
		{
			const Double3 &insidePoint0 = *insidePoints[0];
			const Double3 &insidePoint1 = *insidePoints[1];
			const Double3 &outsidePoint0 = *outsidePoints[0];
			const Double3 &insideNormal0 = *insideNormals[0];
			const Double3 &insideNormal1 = *insideNormals[1];
			const Double3 &outsideNormal0 = *outsideNormals[0];
			const Double2 &insideUV0 = *insideUVs[0];
			const Double2 &insideUV1 = *insideUVs[1];
			const Double2 &outsideUV0 = *outsideUVs[0];

			const Double3 &newTriangle0V0 = insidePoint0;
			const Double3 &newTriangle0V1 = insidePoint1;
			const Double3 &newTriangle0Normal0 = insideNormal0;
			const Double3 &newTriangle0Normal1 = insideNormal1;
			const Double2 &newTriangle0UV0 = insideUV0;
			const Double2 &newTriangle0UV1 = insideUV1;

			const double t0 = (outsidePoint0 - newTriangle0V0).length();

			// @todo: replace ray-plane intersection with one that gets T value internally
			Double3 newTriangle0V2;
			MathUtils::rayPlaneIntersection(newTriangle0V0, (outsidePoint0 - newTriangle0V0).normalized(),
				planePoint, planeNormal, &newTriangle0V2);
			const double newTriangle0T = (newTriangle0V2 - newTriangle0V0).length();
			const Double3 newTriangle0Normal2 = newTriangle0Normal0.lerp(outsideNormal0, newTriangle0T / t0);
			const Double2 newTriangle0UV2 = newTriangle0UV0.lerp(outsideUV0, newTriangle0T / t0);

			const Double3 &newTriangle1V0 = insidePoint1;
			const Double3 &newTriangle1V1 = newTriangle0V2;
			const Double3 &newTriangle1Normal0 = insideNormal1;
			const Double3 &newTriangle1Normal1 = newTriangle0Normal2;
			const Double2 &newTriangle1UV0 = insideUV1;
			const Double2 &newTriangle1UV1 = newTriangle0UV2;

			const double t1 = (outsidePoint0 - newTriangle1V0).length();

			// @todo: replace ray-plane intersection with one that gets T value internally
			Double3 newTriangle1V2;
			MathUtils::rayPlaneIntersection(newTriangle1V0, (outsidePoint0 - newTriangle1V0).normalized(),
				planePoint, planeNormal, &newTriangle1V2);
			const double newTriangle1T = (newTriangle1V2 - newTriangle1V0).length();
			const Double3 newTriangle1Normal2 = newTriangle1Normal0.lerp(outsideNormal0, newTriangle1T / t1);
			const Double2 newTriangle1UV2 = newTriangle1UV0.lerp(outsideUV0, newTriangle1T / t1);

			// Swap vertex winding if needed so we don't generate a back-facing triangle from a front-facing one.
			const Double3 unormal0 = (newTriangle0V0 - newTriangle0V2).cross(newTriangle0V1 - newTriangle0V0);
			const Double3 unormal1 = (newTriangle1V0 - newTriangle1V2).cross(newTriangle1V1 - newTriangle1V0);
			const bool keepOrientation0 = (eye - newTriangle0V0).dot(unormal0) >= Constants::Epsilon;
			const bool keepOrientation1 = (eye - newTriangle1V0).dot(unormal1) >= Constants::Epsilon;
			if (keepOrientation0)
			{
				if (keepOrientation1)
				{
					return TriangleClipResult::two(newTriangle0V0, newTriangle0V1, newTriangle0V2, newTriangle0Normal0, newTriangle0Normal1,
						newTriangle0Normal2, newTriangle0UV0, newTriangle0UV1, newTriangle0UV2, newTriangle1V0, newTriangle1V1, newTriangle1V2,
						newTriangle1Normal0, newTriangle1Normal1, newTriangle1Normal2, newTriangle1UV0, newTriangle1UV1, newTriangle1UV2);
				}
				else
				{
					return TriangleClipResult::two(newTriangle0V0, newTriangle0V1, newTriangle0V2, newTriangle0Normal0, newTriangle0Normal1,
						newTriangle0Normal2, newTriangle0UV0, newTriangle0UV1, newTriangle0UV2, newTriangle1V2, newTriangle1V1, newTriangle1V0,
						newTriangle1Normal2, newTriangle1Normal1, newTriangle1Normal0, newTriangle1UV2, newTriangle1UV1, newTriangle1UV0);
				}
			}
			else
			{
				if (keepOrientation1)
				{
					return TriangleClipResult::two(newTriangle0V2, newTriangle0V1, newTriangle0V0, newTriangle0Normal2, newTriangle0Normal1,
						newTriangle0Normal0, newTriangle0UV2, newTriangle0UV1, newTriangle0UV0, newTriangle1V0, newTriangle1V1, newTriangle1V2,
						newTriangle1Normal0, newTriangle1Normal1, newTriangle0Normal2, newTriangle1UV0, newTriangle1UV1, newTriangle1UV2);
				}
				else
				{
					return TriangleClipResult::two(newTriangle0V2, newTriangle0V1, newTriangle0V0, newTriangle0Normal2, newTriangle0Normal1,
						newTriangle0Normal0, newTriangle0UV2, newTriangle0UV1, newTriangle0UV0, newTriangle1V2, newTriangle1V1, newTriangle1V0,
						newTriangle1Normal2, newTriangle1Normal1, newTriangle1Normal0, newTriangle1UV2, newTriangle1UV1, newTriangle1UV0);
				}
			}
		}
		else
		{
			DebugUnhandledReturnMsg(TriangleClipResult, "Unhandled triangle clip case (inside: " +
				std::to_string(insidePointCount) + ", outside: " + std::to_string(outsidePointCount) + ").");
		}
	}

	// Caches for visible triangle processing/clipping.
	// @optimization: make N of these caches to allow for multi-threaded clipping
	std::vector<Double3> g_visibleTriangleV0s, g_visibleTriangleV1s, g_visibleTriangleV2s;
	std::vector<Double3> g_visibleTriangleNormal0s, g_visibleTriangleNormal1s, g_visibleTriangleNormal2s;
	std::vector<Double2> g_visibleTriangleUV0s, g_visibleTriangleUV1s, g_visibleTriangleUV2s;
	std::vector<ObjectTextureID> g_visibleTriangleTextureID0s, g_visibleTriangleTextureID1s;	
	constexpr int MAX_CLIP_LIST_SIZE = 64; // Arbitrary worst case for processing one triangle. Increase this if clipping breaks (32 wasn't enough).
	std::array<Double3, MAX_CLIP_LIST_SIZE> g_visibleClipListV0s, g_visibleClipListV1s, g_visibleClipListV2s;
	std::array<Double3, MAX_CLIP_LIST_SIZE> g_visibleClipListNormal0s, g_visibleClipListNormal1s, g_visibleClipListNormal2s;
	std::array<Double2, MAX_CLIP_LIST_SIZE> g_visibleClipListUV0s, g_visibleClipListUV1s, g_visibleClipListUV2s;
	std::array<ObjectTextureID, MAX_CLIP_LIST_SIZE> g_visibleClipListTextureID0s, g_visibleClipListTextureID1s;
	int g_visibleTriangleCount = 0; // Note this includes new triangles from clipping.
	int g_totalTriangleCount = 0;
	int g_totalDrawCallCount = 0;

	// Processes the given world space triangles in the following ways, and returns a view to a geometry cache
	// that is invalidated the next time this function is called.
	// 1) Back-face culling
	// 2) Frustum culling
	// 3) Clipping
	swGeometry::TriangleDrawListIndices ProcessMeshForRasterization(const Double3 &modelPosition, const Double3 &preScaleTranslation,
		const Matrix4d &rotation, const Matrix4d &scale, const SoftwareRenderer::VertexBuffer &vertexBuffer,
		const SoftwareRenderer::AttributeBuffer &normalBuffer, const SoftwareRenderer::AttributeBuffer &texCoordBuffer,
		const SoftwareRenderer::IndexBuffer &indexBuffer, ObjectTextureID textureID0, ObjectTextureID textureID1,
		VertexShaderType vertexShaderType, const Double3 &eye, const ClippingPlanes &clippingPlanes)
	{
		std::vector<Double3> &outVisibleTriangleV0s = g_visibleTriangleV0s;
		std::vector<Double3> &outVisibleTriangleV1s = g_visibleTriangleV1s;
		std::vector<Double3> &outVisibleTriangleV2s = g_visibleTriangleV2s;
		std::vector<Double3> &outVisibleTriangleNormal0s = g_visibleTriangleNormal0s;
		std::vector<Double3> &outVisibleTriangleNormal1s = g_visibleTriangleNormal1s;
		std::vector<Double3> &outVisibleTriangleNormal2s = g_visibleTriangleNormal2s;
		std::vector<Double2> &outVisibleTriangleUV0s = g_visibleTriangleUV0s;
		std::vector<Double2> &outVisibleTriangleUV1s = g_visibleTriangleUV1s;
		std::vector<Double2> &outVisibleTriangleUV2s = g_visibleTriangleUV2s;
		std::vector<ObjectTextureID> &outVisibleTriangleTextureID0s = g_visibleTriangleTextureID0s;
		std::vector<ObjectTextureID> &outVisibleTriangleTextureID1s = g_visibleTriangleTextureID1s;
		std::array<Double3, MAX_CLIP_LIST_SIZE> &outClipListV0s = g_visibleClipListV0s;
		std::array<Double3, MAX_CLIP_LIST_SIZE> &outClipListV1s = g_visibleClipListV1s;
		std::array<Double3, MAX_CLIP_LIST_SIZE> &outClipListV2s = g_visibleClipListV2s;
		std::array<Double3, MAX_CLIP_LIST_SIZE> &outClipListNormal0s = g_visibleClipListNormal0s;
		std::array<Double3, MAX_CLIP_LIST_SIZE> &outClipListNormal1s = g_visibleClipListNormal1s;
		std::array<Double3, MAX_CLIP_LIST_SIZE> &outClipListNormal2s = g_visibleClipListNormal2s;
		std::array<Double2, MAX_CLIP_LIST_SIZE> &outClipListUV0s = g_visibleClipListUV0s;
		std::array<Double2, MAX_CLIP_LIST_SIZE> &outClipListUV1s = g_visibleClipListUV1s;
		std::array<Double2, MAX_CLIP_LIST_SIZE> &outClipListUV2s = g_visibleClipListUV2s;
		std::array<ObjectTextureID, MAX_CLIP_LIST_SIZE> &outClipListTextureID0s = g_visibleClipListTextureID0s;
		std::array<ObjectTextureID, MAX_CLIP_LIST_SIZE> &outClipListTextureID1s = g_visibleClipListTextureID1s;
		int *outVisibleTriangleCount = &g_visibleTriangleCount;
		int *outTotalTriangleCount = &g_totalTriangleCount;

		outVisibleTriangleV0s.clear();
		outVisibleTriangleV1s.clear();
		outVisibleTriangleV2s.clear();
		outVisibleTriangleNormal0s.clear();
		outVisibleTriangleNormal1s.clear();
		outVisibleTriangleNormal2s.clear();
		outVisibleTriangleUV0s.clear();
		outVisibleTriangleUV1s.clear();
		outVisibleTriangleUV2s.clear();
		outVisibleTriangleTextureID0s.clear();
		outVisibleTriangleTextureID1s.clear();

		const Double4 modelPositionXYZW(modelPosition, 0.0);
		const Double4 preScaleTranslationXYZW(preScaleTranslation, 1.0);

		const double *verticesPtr = vertexBuffer.vertices.get();
		const double *normalsPtr = normalBuffer.attributes.get();
		const double *texCoordsPtr = texCoordBuffer.attributes.get();
		const int32_t *indicesPtr = indexBuffer.indices.get();
		const int triangleCount = indexBuffer.indices.getCount() / 3;
		for (int i = 0; i < triangleCount; i++)
		{
			const int indexBufferBase = i * 3;
			const int32_t index0 = indicesPtr[indexBufferBase];
			const int32_t index1 = indicesPtr[indexBufferBase + 1];
			const int32_t index2 = indicesPtr[indexBufferBase + 2];
			const int32_t v0Index = index0 * 3;
			const int32_t v1Index = index1 * 3;
			const int32_t v2Index = index2 * 3;
			const int32_t normal0Index = v0Index;
			const int32_t normal1Index = v1Index;
			const int32_t normal2Index = v2Index;

			const Double4 unshadedV0(
				*(verticesPtr + v0Index),
				*(verticesPtr + v0Index + 1),
				*(verticesPtr + v0Index + 2),
				1.0);
			const Double4 unshadedV1(
				*(verticesPtr + v1Index),
				*(verticesPtr + v1Index + 1),
				*(verticesPtr + v1Index + 2),
				1.0);
			const Double4 unshadedV2(
				*(verticesPtr + v2Index),
				*(verticesPtr + v2Index + 1),
				*(verticesPtr + v2Index + 2),
				1.0);
			const Double4 unshadedNormal0(
				*(normalsPtr + normal0Index),
				*(normalsPtr + normal0Index + 1),
				*(normalsPtr + normal0Index + 2),
				0.0);
			const Double4 unshadedNormal1(
				*(normalsPtr + normal1Index),
				*(normalsPtr + normal1Index + 1),
				*(normalsPtr + normal1Index + 2),
				0.0);
			const Double4 unshadedNormal2(
				*(normalsPtr + normal2Index),
				*(normalsPtr + normal2Index + 1),
				*(normalsPtr + normal2Index + 2),
				0.0);

			Double4 shadedV0, shadedV1, shadedV2;
			Double4 shadedNormal0, shadedNormal1, shadedNormal2;
			switch (vertexShaderType)
			{
			case VertexShaderType::Voxel:
				shadedV0 = unshadedV0 + modelPositionXYZW;
				shadedV1 = unshadedV1 + modelPositionXYZW;
				shadedV2 = unshadedV2 + modelPositionXYZW;
				shadedNormal0 = unshadedNormal0;
				shadedNormal1 = unshadedNormal1;
				shadedNormal2 = unshadedNormal2;
				break;
			case VertexShaderType::SwingingDoor:
				shadedV0 = (rotation * unshadedV0) + modelPositionXYZW;
				shadedV1 = (rotation * unshadedV1) + modelPositionXYZW;
				shadedV2 = (rotation * unshadedV2) + modelPositionXYZW;
				shadedNormal0 = rotation * unshadedNormal0;
				shadedNormal1 = rotation * unshadedNormal1;
				shadedNormal2 = rotation * unshadedNormal2;
				break;
			case VertexShaderType::SlidingDoor:
				shadedV0 = (rotation * (scale * unshadedV0)) + modelPositionXYZW;
				shadedV1 = (rotation * (scale * unshadedV1)) + modelPositionXYZW;
				shadedV2 = (rotation * (scale * unshadedV2)) + modelPositionXYZW;
				shadedNormal0 = rotation * unshadedNormal0;
				shadedNormal1 = rotation * unshadedNormal1;
				shadedNormal2 = rotation * unshadedNormal2;
				break;
			case VertexShaderType::RaisingDoor:
				// Need to push + pop a translation so it scales towards the ceiling.
				shadedV0 = unshadedV0 + preScaleTranslationXYZW;
				shadedV1 = unshadedV1 + preScaleTranslationXYZW;
				shadedV2 = unshadedV2 + preScaleTranslationXYZW;
				shadedV0 = scale * shadedV0;
				shadedV1 = scale * shadedV1;
				shadedV2 = scale * shadedV2;
				shadedV0 = shadedV0 - preScaleTranslationXYZW;
				shadedV1 = shadedV1 - preScaleTranslationXYZW;
				shadedV2 = shadedV2 - preScaleTranslationXYZW;

				shadedV0 = (rotation * shadedV0) + modelPositionXYZW;
				shadedV1 = (rotation * shadedV1) + modelPositionXYZW;
				shadedV2 = (rotation * shadedV2) + modelPositionXYZW;
				shadedNormal0 = rotation * unshadedNormal0;
				shadedNormal1 = rotation * unshadedNormal1;
				shadedNormal2 = rotation * unshadedNormal2;
				break;
			case VertexShaderType::SplittingDoor:
			{
				DebugNotImplemented();
				break;
			}
			default:
				DebugNotImplementedMsg(std::to_string(static_cast<int>(vertexShaderType)));
				break;
			}

			const Double3 shadedV0XYZ(shadedV0.x, shadedV0.y, shadedV0.z);
			const Double3 shadedV1XYZ(shadedV1.x, shadedV1.y, shadedV1.z);
			const Double3 shadedV2XYZ(shadedV2.x, shadedV2.y, shadedV2.z);
			const Double3 shadedNormal0XYZ(shadedNormal0.x, shadedNormal0.y, shadedNormal0.z);
			const Double3 shadedNormal1XYZ(shadedNormal1.x, shadedNormal1.y, shadedNormal1.z);
			const Double3 shadedNormal2XYZ(shadedNormal2.x, shadedNormal2.y, shadedNormal2.z);
			const Double2 uv0(
				*(texCoordsPtr + (index0 * 2)),
				*(texCoordsPtr + (index0 * 2) + 1));
			const Double2 uv1(
				*(texCoordsPtr + (index1 * 2)),
				*(texCoordsPtr + (index1 * 2) + 1));
			const Double2 uv2(
				*(texCoordsPtr + (index2 * 2)),
				*(texCoordsPtr + (index2 * 2) + 1));

			// Discard back-facing.
			const Double3 v0ToEye = eye - shadedV0XYZ;
			if (v0ToEye.dot(shadedNormal0XYZ) < Constants::Epsilon)
			{
				continue;
			}

			// Manually update clip list size and index 0 instead of doing costly vector resizing.
			int clipListSize = 1;
			int clipListFrontIndex = 0;

			// Add the first triangle to clip.
			outClipListV0s[clipListFrontIndex] = shadedV0XYZ;
			outClipListV1s[clipListFrontIndex] = shadedV1XYZ;
			outClipListV2s[clipListFrontIndex] = shadedV2XYZ;
			outClipListNormal0s[clipListFrontIndex] = shadedNormal0XYZ;
			outClipListNormal1s[clipListFrontIndex] = shadedNormal1XYZ;
			outClipListNormal2s[clipListFrontIndex] = shadedNormal2XYZ;
			outClipListUV0s[clipListFrontIndex] = uv0;
			outClipListUV1s[clipListFrontIndex] = uv1;
			outClipListUV2s[clipListFrontIndex] = uv2;
			outClipListTextureID0s[clipListFrontIndex] = textureID0;
			outClipListTextureID1s[clipListFrontIndex] = textureID1;

			for (const ClippingPlane &plane : clippingPlanes)
			{
				const int trianglesToClipCount = clipListSize - clipListFrontIndex;
				for (int j = trianglesToClipCount; j > 0; j--)
				{
					const Double3 &clipListV0 = outClipListV0s[clipListFrontIndex];
					const Double3 &clipListV1 = outClipListV1s[clipListFrontIndex];
					const Double3 &clipListV2 = outClipListV2s[clipListFrontIndex];
					const Double3 &clipListNormal0 = outClipListNormal0s[clipListFrontIndex];
					const Double3 &clipListNormal1 = outClipListNormal1s[clipListFrontIndex];
					const Double3 &clipListNormal2 = outClipListNormal2s[clipListFrontIndex];
					const Double2 &clipListUV0 = outClipListUV0s[clipListFrontIndex];
					const Double2 &clipListUV1 = outClipListUV1s[clipListFrontIndex];
					const Double2 &clipListUV2 = outClipListUV2s[clipListFrontIndex];
					const ObjectTextureID clipListTextureID0 = outClipListTextureID0s[clipListFrontIndex];
					const ObjectTextureID clipListTextureID1 = outClipListTextureID1s[clipListFrontIndex];

					const TriangleClipResult clipResult = ClipTriangle(clipListV0, clipListV1, clipListV2,
						clipListNormal0, clipListNormal1, clipListNormal2, clipListUV0, clipListUV1, clipListUV2,
						eye, plane.point, plane.normal);

					if (clipResult.triangleCount > 0)
					{
						const int oldClipListSize = clipListSize;
						const int newClipListSize = oldClipListSize + clipResult.triangleCount;
						DebugAssert(newClipListSize < swGeometry::MAX_CLIP_LIST_SIZE);

						const int dstIndex = newClipListSize - clipResult.triangleCount;
						std::memcpy(&outClipListV0s[dstIndex], clipResult.v0s, clipResult.triangleCount * sizeof(clipResult.v0s[0]));
						std::memcpy(&outClipListV1s[dstIndex], clipResult.v1s, clipResult.triangleCount * sizeof(clipResult.v1s[0]));
						std::memcpy(&outClipListV2s[dstIndex], clipResult.v2s, clipResult.triangleCount * sizeof(clipResult.v2s[0]));
						std::memcpy(&outClipListNormal0s[dstIndex], clipResult.normal0s, clipResult.triangleCount * sizeof(clipResult.normal0s[0]));
						std::memcpy(&outClipListNormal1s[dstIndex], clipResult.normal1s, clipResult.triangleCount * sizeof(clipResult.normal1s[0]));
						std::memcpy(&outClipListNormal2s[dstIndex], clipResult.normal2s, clipResult.triangleCount * sizeof(clipResult.normal2s[0]));
						std::memcpy(&outClipListUV0s[dstIndex], clipResult.uv0s, clipResult.triangleCount * sizeof(clipResult.uv0s[0]));
						std::memcpy(&outClipListUV1s[dstIndex], clipResult.uv1s, clipResult.triangleCount * sizeof(clipResult.uv1s[0]));
						std::memcpy(&outClipListUV2s[dstIndex], clipResult.uv2s, clipResult.triangleCount * sizeof(clipResult.uv2s[0]));
						
						for (int clipResultIndex = 0; clipResultIndex < clipResult.triangleCount; clipResultIndex++)
						{
							const int writeIndex = dstIndex + clipResultIndex;
							outClipListTextureID0s[writeIndex] = textureID0;
							outClipListTextureID1s[writeIndex] = textureID1;
						}

						clipListSize = newClipListSize;
					}

					clipListFrontIndex++;
				}
			}

			// Append newly clipped triangles to visible triangles (faster than vector::insert or std::copy in debug build).
			const int oldVisibleTrianglesCount = static_cast<int>(outVisibleTriangleV0s.size());
			const int newlyClippedTrianglesCount = static_cast<int>(std::distance(outClipListV0s.begin() + clipListFrontIndex, outClipListV0s.begin() + clipListSize));
			const int totalTrianglesCount = oldVisibleTrianglesCount + newlyClippedTrianglesCount;

			if (totalTrianglesCount > oldVisibleTrianglesCount)
			{
				outVisibleTriangleV0s.resize(totalTrianglesCount);
				outVisibleTriangleV1s.resize(totalTrianglesCount);
				outVisibleTriangleV2s.resize(totalTrianglesCount);
				outVisibleTriangleNormal0s.resize(totalTrianglesCount);
				outVisibleTriangleNormal1s.resize(totalTrianglesCount);
				outVisibleTriangleNormal2s.resize(totalTrianglesCount);
				outVisibleTriangleUV0s.resize(totalTrianglesCount);
				outVisibleTriangleUV1s.resize(totalTrianglesCount);
				outVisibleTriangleUV2s.resize(totalTrianglesCount);
				outVisibleTriangleTextureID0s.resize(totalTrianglesCount);
				outVisibleTriangleTextureID1s.resize(totalTrianglesCount);

				std::memcpy(&outVisibleTriangleV0s[oldVisibleTrianglesCount], &outClipListV0s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListV0s[0]));
				std::memcpy(&outVisibleTriangleV1s[oldVisibleTrianglesCount], &outClipListV1s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListV1s[0]));
				std::memcpy(&outVisibleTriangleV2s[oldVisibleTrianglesCount], &outClipListV2s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListV2s[0]));
				std::memcpy(&outVisibleTriangleNormal0s[oldVisibleTrianglesCount], &outClipListNormal0s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListNormal0s[0]));
				std::memcpy(&outVisibleTriangleNormal1s[oldVisibleTrianglesCount], &outClipListNormal1s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListNormal1s[0]));
				std::memcpy(&outVisibleTriangleNormal2s[oldVisibleTrianglesCount], &outClipListNormal2s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListNormal2s[0]));
				std::memcpy(&outVisibleTriangleUV0s[oldVisibleTrianglesCount], &outClipListUV0s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListUV0s[0]));
				std::memcpy(&outVisibleTriangleUV1s[oldVisibleTrianglesCount], &outClipListUV1s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListUV1s[0]));
				std::memcpy(&outVisibleTriangleUV2s[oldVisibleTrianglesCount], &outClipListUV2s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListUV2s[0]));
				std::memcpy(&outVisibleTriangleTextureID0s[oldVisibleTrianglesCount], &outClipListTextureID0s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListTextureID0s[0]));
				std::memcpy(&outVisibleTriangleTextureID1s[oldVisibleTrianglesCount], &outClipListTextureID1s[clipListFrontIndex], newlyClippedTrianglesCount * sizeof(outClipListTextureID1s[0]));
			}
		}
		
		const int visibleTriangleCount = static_cast<int>(outVisibleTriangleV0s.size());
		*outVisibleTriangleCount += visibleTriangleCount;
		*outTotalTriangleCount += triangleCount;
		return swGeometry::TriangleDrawListIndices(0, visibleTriangleCount); // All visible triangles.
	}
}

// Rendering functions, per-pixel work.
namespace swRender
{
	void DrawDebugRGB(const RenderCamera &camera, BufferView2D<uint32_t> &colorBuffer)
	{
		const int frameBufferWidth = colorBuffer.getWidth();
		const int frameBufferHeight = colorBuffer.getHeight();
		uint32_t *colorBufferPtr = colorBuffer.get();

		for (int y = 0; y < frameBufferHeight; y++)
		{
			const double yPercent = (static_cast<double>(y) + 0.50) / static_cast<double>(frameBufferHeight);

			for (int x = 0; x < frameBufferWidth; x++)
			{
				const double xPercent = (static_cast<double>(x) + 0.50) / static_cast<double>(frameBufferWidth);

				const Double3 pixelDir = ((camera.forwardScaled - camera.rightScaled + camera.up) +
					(camera.rightScaled * (xPercent * 2.0)) - (camera.up * (yPercent * 2.0))).normalized();

				const Double3 pixelDirClamped(
					std::max(pixelDir.x, 0.0),
					std::max(pixelDir.y, 0.0),
					std::max(pixelDir.z, 0.0));

				const Color color(
					static_cast<uint8_t>(pixelDirClamped.x * 255.0),
					static_cast<uint8_t>(pixelDirClamped.y * 255.0),
					static_cast<uint8_t>(pixelDirClamped.z * 255.0));

				const uint32_t outputColor = color.toARGB();
				const int outputIndex = x + (y * frameBufferWidth);
				colorBufferPtr[outputIndex] = outputColor;
			}
		}
	}

	void ClearFrameBuffers(uint32_t clearColor, BufferView2D<uint32_t> &colorBuffer, BufferView2D<double> &depthBuffer)
	{
		colorBuffer.fill(clearColor);
		depthBuffer.fill(std::numeric_limits<double>::infinity());
	}

	void ClearTriangleDrawList()
	{
		swGeometry::g_visibleTriangleV0s.clear();
		swGeometry::g_visibleTriangleV1s.clear();
		swGeometry::g_visibleTriangleV2s.clear();
		swGeometry::g_visibleTriangleNormal0s.clear();
		swGeometry::g_visibleTriangleNormal1s.clear();
		swGeometry::g_visibleTriangleNormal2s.clear();
		swGeometry::g_visibleTriangleUV0s.clear();
		swGeometry::g_visibleTriangleUV1s.clear();
		swGeometry::g_visibleTriangleUV2s.clear();
		swGeometry::g_visibleTriangleTextureID0s.clear();
		swGeometry::g_visibleTriangleTextureID1s.clear();
		swGeometry::g_visibleClipListV0s.fill(Double3::Zero);
		swGeometry::g_visibleClipListV1s.fill(Double3::Zero);
		swGeometry::g_visibleClipListV2s.fill(Double3::Zero);
		swGeometry::g_visibleClipListNormal0s.fill(Double3::Zero);
		swGeometry::g_visibleClipListNormal1s.fill(Double3::Zero);
		swGeometry::g_visibleClipListNormal2s.fill(Double3::Zero);
		swGeometry::g_visibleClipListUV0s.fill(Double2::Zero);
		swGeometry::g_visibleClipListUV1s.fill(Double2::Zero);
		swGeometry::g_visibleClipListUV2s.fill(Double2::Zero);
		swGeometry::g_visibleClipListTextureID0s.fill(-1);
		swGeometry::g_visibleClipListTextureID1s.fill(-1);
		swGeometry::g_visibleTriangleCount = 0;
		swGeometry::g_totalTriangleCount = 0;
	}

	struct PixelShaderPerspectiveCorrection
	{
		double depth;
		Double2 texelPercent;
	};

	struct PixelShaderTexture
	{
		const uint8_t *texels;
		int width;
		int height;
		TextureSamplingType samplingType;
	};

	struct PixelShaderPalette
	{
		const uint32_t *colors;
		int count;
	};

	struct PixelShaderFrameBuffer
	{
		uint32_t *colors;
		double *depth;
		PixelShaderPalette palette;
		double xPercent, yPercent;
		int pixelIndex;
	};

	// @todo: lighting/shading
	/*const int lightLevelTexelCount = lightTableTexture.texels.getWidth(); // Per light level, not the whole table.
	const int lightLevelCount = lightTableTexture.texels.getHeight();
	const double lightLevelCountReal = static_cast<double>(lightLevelCount);
	const uint8_t *lightLevelTexels = lightTableTexture.texels.get();*/

	/*double shadingPercent;
	if (isFading)
	{
		shadingPercent = fadePercent;
	}
	else
	{
		// @todo: fix interpolated world space point calculation
		// XZ position of pixel center in world space.
		//const Double2 v2D(
		//	(u * v0.x) + (v * v1.x) + (w * v2.x),
		//	(u * v0.z) + (v * v1.z) + (w * v2.z));
		//const double distanceToLight = (v2D - eye2D).length();
		//shadingPercent = std::clamp(distanceToLight / swConstants::PLAYER_LIGHT_DISTANCE, 0.0, 1.0);
		//shadingPercent = 0.0; // Full-bright
	}

	const double lightLevelValue = shadingPercent * lightLevelCountReal;

	// Index into light table palettes.
	const int lightLevelIndex = std::clamp(static_cast<int>(lightLevelValue), 0, lightLevelCount - 1);

	// Percent through the current light level.
	const double lightLevelPercent = std::clamp(lightLevelValue - std::floor(lightLevelValue), 0.0, Constants::JustBelowOne);

	const int shadedTexelIndex = texel + (lightLevelIndex * lightLevelTexelCount);
	const uint8_t shadedTexel = lightLevelTexels[shadedTexelIndex];
	const uint32_t shadedTexelColor = paletteTexels[shadedTexel];

	colorBufferPtr[outputIndex] = shadedTexelColor;
	depthBufferPtr[outputIndex] = depth;*/

	// @todo chasms: determine how many pixels the original texture should cover, based on what percentage the original texture height is over the original screen height.
	void PixelShader_Opaque(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture, PixelShaderFrameBuffer &frameBuffer)
	{
		int texelX = -1;
		int texelY = -1;
		if (texture.samplingType == TextureSamplingType::Default)
		{
			texelX = std::clamp(static_cast<int>(perspective.texelPercent.x * texture.width), 0, texture.width - 1);
			texelY = std::clamp(static_cast<int>(perspective.texelPercent.y * texture.height), 0, texture.height - 1);
		}
		else if (texture.samplingType == TextureSamplingType::ScreenSpaceRepeatY)
		{
			texelX = std::clamp(static_cast<int>(frameBuffer.xPercent * texture.width), 0, texture.width - 1);

			const double v = frameBuffer.yPercent * 2.0;
			const double actualV = v >= 1.0 ? (v - 1.0) : v;
			texelY = std::clamp(static_cast<int>(actualV * texture.height), 0, texture.height - 1);
		}

		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];
		const uint32_t color = frameBuffer.palette.colors[texel];
		frameBuffer.colors[frameBuffer.pixelIndex] = color;
		frameBuffer.depth[frameBuffer.pixelIndex] = perspective.depth;
	}

	void PixelShader_OpaqueWithAlphaTestLayer(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &opaqueTexture,
		const PixelShaderTexture &alphaTestTexture, PixelShaderFrameBuffer &frameBuffer)
	{
		const int layerTexelX = std::clamp(static_cast<int>(perspective.texelPercent.x * alphaTestTexture.width), 0, alphaTestTexture.width - 1);
		const int layerTexelY = std::clamp(static_cast<int>(perspective.texelPercent.y * alphaTestTexture.height), 0, alphaTestTexture.height - 1);
		const int layerTexelIndex = layerTexelX + (layerTexelY * alphaTestTexture.width);
		uint8_t texel = alphaTestTexture.texels[layerTexelIndex];

		const bool isTransparent = texel == 0;
		if (isTransparent)
		{
			const int texelX = std::clamp(static_cast<int>(frameBuffer.xPercent * opaqueTexture.width), 0, opaqueTexture.width - 1);

			const double v = frameBuffer.yPercent * 2.0;
			const double actualV = v >= 1.0 ? (v - 1.0) : v;
			const int texelY = std::clamp(static_cast<int>(actualV * opaqueTexture.height), 0, opaqueTexture.height - 1);

			const int texelIndex = texelX + (texelY * opaqueTexture.width);
			texel = opaqueTexture.texels[texelIndex];
		}

		const uint32_t color = frameBuffer.palette.colors[texel];
		frameBuffer.colors[frameBuffer.pixelIndex] = color;
		frameBuffer.depth[frameBuffer.pixelIndex] = perspective.depth;
	}

	void PixelShader_AlphaTest(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercent.x * texture.width), 0, texture.width - 1);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercent.y * texture.height), 0, texture.height - 1);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == 0;
		if (isTransparent)
		{
			return;
		}

		const uint32_t color = frameBuffer.palette.colors[texel];
		frameBuffer.colors[frameBuffer.pixelIndex] = color;
		frameBuffer.depth[frameBuffer.pixelIndex] = perspective.depth;
	}

	void PixelShader_AlphaTestedWithVariableTexCoordUMin(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		double uMin, PixelShaderFrameBuffer &frameBuffer)
	{
		const double u = std::clamp(uMin + ((1.0 - uMin) * perspective.texelPercent.x), uMin, 1.0);
		const int texelX = std::clamp(static_cast<int>(u * texture.width), 0, texture.width - 1);
		const int texelY = std::clamp(static_cast<int>(perspective.texelPercent.y * texture.height), 0, texture.height - 1);
		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == 0;
		if (isTransparent)
		{
			return;
		}

		const uint32_t color = frameBuffer.palette.colors[texel];
		frameBuffer.colors[frameBuffer.pixelIndex] = color;
		frameBuffer.depth[frameBuffer.pixelIndex] = perspective.depth;
	}

	void PixelShader_AlphaTestedWithVariableTexCoordVMin(const PixelShaderPerspectiveCorrection &perspective, const PixelShaderTexture &texture,
		double vMin, PixelShaderFrameBuffer &frameBuffer)
	{
		const int texelX = std::clamp(static_cast<int>(perspective.texelPercent.x * texture.width), 0, texture.width - 1);
		const double v = std::clamp(vMin + ((1.0 - vMin) * perspective.texelPercent.y), vMin, 1.0);
		const int texelY = std::clamp(static_cast<int>(v * texture.height), 0, texture.height - 1);

		const int texelIndex = texelX + (texelY * texture.width);
		const uint8_t texel = texture.texels[texelIndex];

		const bool isTransparent = texel == 0;
		if (isTransparent)
		{
			return;
		}

		const uint32_t color = frameBuffer.palette.colors[texel];
		frameBuffer.colors[frameBuffer.pixelIndex] = color;
		frameBuffer.depth[frameBuffer.pixelIndex] = perspective.depth;
	}

	// The provided triangles are assumed to be back-face culled and clipped.
	void RasterizeTriangles(const swGeometry::TriangleDrawListIndices &drawListIndices, TextureSamplingType textureSamplingType,
		PixelShaderType pixelShaderType, double pixelShaderParam0, const SoftwareRenderer::ObjectTexturePool &textures,
		const SoftwareRenderer::ObjectTexture &paletteTexture, const SoftwareRenderer::ObjectTexture &lightTableTexture,
		const RenderCamera &camera, BufferView2D<uint32_t> &colorBuffer, BufferView2D<double> &depthBuffer)
	{
		const int frameBufferWidth = colorBuffer.getWidth();
		const int frameBufferHeight = colorBuffer.getHeight();
		const double frameBufferWidthReal = static_cast<double>(frameBufferWidth);
		const double frameBufferHeightReal = static_cast<double>(frameBufferHeight);

		const Double2 eye2D(camera.worldPoint.x, camera.worldPoint.z); // For 2D lighting.
		const Matrix4d &viewMatrix = camera.viewMatrix;
		const Matrix4d &perspectiveMatrix = camera.perspectiveMatrix;

		constexpr double yShear = 0.0;

		const int triangleCount = drawListIndices.count;
		for (int i = 0; i < triangleCount; i++)
		{
			const int index = drawListIndices.startIndex + i;
			const Double3 &v0 = swGeometry::g_visibleTriangleV0s[index];
			const Double3 &v1 = swGeometry::g_visibleTriangleV1s[index];
			const Double3 &v2 = swGeometry::g_visibleTriangleV2s[index];
			const Double4 view0 = RendererUtils::worldSpaceToCameraSpace(Double4(v0, 1.0), viewMatrix);
			const Double4 view1 = RendererUtils::worldSpaceToCameraSpace(Double4(v1, 1.0), viewMatrix);
			const Double4 view2 = RendererUtils::worldSpaceToCameraSpace(Double4(v2, 1.0), viewMatrix);
			const Double4 clip0 = RendererUtils::cameraSpaceToClipSpace(view0, perspectiveMatrix);
			const Double4 clip1 = RendererUtils::cameraSpaceToClipSpace(view1, perspectiveMatrix);
			const Double4 clip2 = RendererUtils::cameraSpaceToClipSpace(view2, perspectiveMatrix);
			const Double3 ndc0 = RendererUtils::clipSpaceToNDC(clip0);
			const Double3 ndc1 = RendererUtils::clipSpaceToNDC(clip1);
			const Double3 ndc2 = RendererUtils::clipSpaceToNDC(clip2);
			const Double3 screenSpace0 = RendererUtils::ndcToScreenSpace(ndc0, yShear, frameBufferWidthReal, frameBufferHeightReal);
			const Double3 screenSpace1 = RendererUtils::ndcToScreenSpace(ndc1, yShear, frameBufferWidthReal, frameBufferHeightReal);
			const Double3 screenSpace2 = RendererUtils::ndcToScreenSpace(ndc2, yShear, frameBufferWidthReal, frameBufferHeightReal);
			const Double2 screenSpace0_2D(screenSpace0.x, screenSpace0.y);
			const Double2 screenSpace1_2D(screenSpace1.x, screenSpace1.y);
			const Double2 screenSpace2_2D(screenSpace2.x, screenSpace2.y);
			const Double2 screenSpace01 = screenSpace1_2D - screenSpace0_2D;
			const Double2 screenSpace12 = screenSpace2_2D - screenSpace1_2D;
			const Double2 screenSpace20 = screenSpace0_2D - screenSpace2_2D;
			const Double2 screenSpace01Perp = screenSpace01.rightPerp();
			const Double2 screenSpace12Perp = screenSpace12.rightPerp();
			const Double2 screenSpace20Perp = screenSpace20.rightPerp();

			// Naive screen-space bounding box around triangle.
			const double xMin = std::min(screenSpace0.x, std::min(screenSpace1.x, screenSpace2.x));
			const double xMax = std::max(screenSpace0.x, std::max(screenSpace1.x, screenSpace2.x));
			const double yMin = std::min(screenSpace0.y, std::min(screenSpace1.y, screenSpace2.y));
			const double yMax = std::max(screenSpace0.y, std::max(screenSpace1.y, screenSpace2.y));
			const int xStart = RendererUtils::getLowerBoundedPixel(xMin, frameBufferWidth);
			const int xEnd = RendererUtils::getUpperBoundedPixel(xMax, frameBufferWidth);
			const int yStart = RendererUtils::getLowerBoundedPixel(yMin, frameBufferHeight);
			const int yEnd = RendererUtils::getUpperBoundedPixel(yMax, frameBufferHeight);

			const double z0 = view0.z;
			const double z1 = view1.z;
			const double z2 = view2.z;
			const double z0Recip = 1.0 / z0;
			const double z1Recip = 1.0 / z1;
			const double z2Recip = 1.0 / z2;

			const Double2 &uv0 = swGeometry::g_visibleTriangleUV0s[index];
			const Double2 &uv1 = swGeometry::g_visibleTriangleUV1s[index];
			const Double2 &uv2 = swGeometry::g_visibleTriangleUV2s[index];
			const Double2 uv0Perspective = uv0 * z0Recip;
			const Double2 uv1Perspective = uv1 * z1Recip;
			const Double2 uv2Perspective = uv2 * z2Recip;

			const ObjectTextureID textureID0 = swGeometry::g_visibleTriangleTextureID0s[index];
			const ObjectTextureID textureID1 = swGeometry::g_visibleTriangleTextureID1s[index];
			const bool isMultiTextured = pixelShaderType == PixelShaderType::OpaqueWithAlphaTestLayer;
			const SoftwareRenderer::ObjectTexture &texture0 = textures.get(textureID0);
			const SoftwareRenderer::ObjectTexture &texture1 = isMultiTextured ? textures.get(textureID1) : texture0;

			PixelShaderTexture shaderTexture0;
			shaderTexture0.texels = texture0.texels.get();
			shaderTexture0.width = texture0.texels.getWidth();
			shaderTexture0.height = texture0.texels.getHeight();
			shaderTexture0.samplingType = textureSamplingType;

			PixelShaderTexture shaderTexture1;
			shaderTexture1.texels = texture1.texels.get();
			shaderTexture1.width = texture1.texels.getWidth();
			shaderTexture1.height = texture1.texels.getHeight();
			shaderTexture1.samplingType = TextureSamplingType::ScreenSpaceRepeatY; // @todo: change to variable once other shaders need this

			const double fadePercent = 0.0; // @todo
			const bool isFading = fadePercent > 0.0;

			PixelShaderFrameBuffer shaderFrameBuffer;
			shaderFrameBuffer.colors = colorBuffer.get();
			shaderFrameBuffer.depth = depthBuffer.get();
			shaderFrameBuffer.palette.colors = paletteTexture.paletteTexels.get();
			shaderFrameBuffer.palette.count = paletteTexture.paletteTexels.getCount();

			for (int y = yStart; y < yEnd; y++)
			{
				shaderFrameBuffer.yPercent = (static_cast<double>(y) + 0.50) / frameBufferHeightReal;

				for (int x = xStart; x < xEnd; x++)
				{
					shaderFrameBuffer.xPercent = (static_cast<double>(x) + 0.50) / frameBufferWidthReal;
					const Double2 pixelCenter(
						shaderFrameBuffer.xPercent * frameBufferWidthReal,
						shaderFrameBuffer.yPercent * frameBufferHeightReal);

					// See if pixel center is inside triangle.
					const bool inHalfSpace0 = MathUtils::isPointInHalfSpace(pixelCenter, screenSpace0_2D, screenSpace01Perp);
					const bool inHalfSpace1 = MathUtils::isPointInHalfSpace(pixelCenter, screenSpace1_2D, screenSpace12Perp);
					const bool inHalfSpace2 = MathUtils::isPointInHalfSpace(pixelCenter, screenSpace2_2D, screenSpace20Perp);
					if (inHalfSpace0 && inHalfSpace1 && inHalfSpace2)
					{
						const Double2 &ss0 = screenSpace01;
						const Double2 ss1 = screenSpace2_2D - screenSpace0_2D;
						const Double2 ss2 = pixelCenter - screenSpace0_2D;

						const double dot00 = ss0.dot(ss0);
						const double dot01 = ss0.dot(ss1);
						const double dot11 = ss1.dot(ss1);
						const double dot20 = ss2.dot(ss0);
						const double dot21 = ss2.dot(ss1);
						const double denominator = (dot00 * dot11) - (dot01 * dot01);

						const double v = ((dot11 * dot20) - (dot01 * dot21)) / denominator;
						const double w = ((dot00 * dot21) - (dot01 * dot20)) / denominator;
						const double u = 1.0 - v - w;

						PixelShaderPerspectiveCorrection shaderPerspective;
						shaderPerspective.depth = 1.0 / ((u * z0Recip) + (v * z1Recip) + (w * z2Recip));

						shaderFrameBuffer.pixelIndex = x + (y * frameBufferWidth);
						if (shaderPerspective.depth < shaderFrameBuffer.depth[shaderFrameBuffer.pixelIndex])
						{
							shaderPerspective.texelPercent.x = ((u * uv0Perspective.x) + (v * uv1Perspective.x) + (w * uv2Perspective.x)) / ((u * z0Recip) + (v * z1Recip) + (w * z2Recip));
							shaderPerspective.texelPercent.y = ((u * uv0Perspective.y) + (v * uv1Perspective.y) + (w * uv2Perspective.y)) / ((u * z0Recip) + (v * z1Recip) + (w * z2Recip));

							switch (pixelShaderType)
							{
							case PixelShaderType::Opaque:
								PixelShader_Opaque(shaderPerspective, shaderTexture0, shaderFrameBuffer);
								break;
							case PixelShaderType::OpaqueWithAlphaTestLayer:
								PixelShader_OpaqueWithAlphaTestLayer(shaderPerspective, shaderTexture0, shaderTexture1, shaderFrameBuffer);
								break;
							case PixelShaderType::AlphaTested:
								PixelShader_AlphaTest(shaderPerspective, shaderTexture0, shaderFrameBuffer);
								break;
							case PixelShaderType::AlphaTestedWithVariableTexCoordUMin:
								PixelShader_AlphaTestedWithVariableTexCoordUMin(shaderPerspective, shaderTexture0, pixelShaderParam0, shaderFrameBuffer);
								break;
							case PixelShaderType::AlphaTestedWithVariableTexCoordVMin:
								PixelShader_AlphaTestedWithVariableTexCoordVMin(shaderPerspective, shaderTexture0, pixelShaderParam0, shaderFrameBuffer);
								break;
							default:
								DebugNotImplementedMsg(std::to_string(static_cast<int>(pixelShaderType)));
								break;
							}
						}
					}
				}
			}
		}
	}
}

void SoftwareRenderer::ObjectTexture::init8Bit(int width, int height)
{
	this->texels.init(width, height);
}

void SoftwareRenderer::ObjectTexture::initPalette(int count)
{
	this->paletteTexels.init(count);
}

void SoftwareRenderer::ObjectTexture::clear()
{
	this->texels.clear();
	this->paletteTexels.clear();
}

void SoftwareRenderer::VertexBuffer::init(int vertexCount, int componentsPerVertex)
{
	const int valueCount = vertexCount * componentsPerVertex;
	this->vertices.init(valueCount);
}

void SoftwareRenderer::AttributeBuffer::init(int vertexCount, int componentsPerVertex)
{
	const int valueCount = vertexCount * componentsPerVertex;
	this->attributes.init(valueCount);
}

void SoftwareRenderer::IndexBuffer::init(int indexCount)
{
	this->indices.init(indexCount);
}

SoftwareRenderer::SoftwareRenderer()
{

}

SoftwareRenderer::~SoftwareRenderer()
{

}

void SoftwareRenderer::init(const RenderInitSettings &settings)
{
	this->depthBuffer.init(settings.width, settings.height);
}

void SoftwareRenderer::shutdown()
{
	this->depthBuffer.clear();
	this->vertexBuffers.clear();
	this->attributeBuffers.clear();
	this->indexBuffers.clear();
	this->objectTextures.clear();
}

bool SoftwareRenderer::isInited() const
{
	return true;
}

void SoftwareRenderer::resize(int width, int height)
{
	this->depthBuffer.init(width, height);
	this->depthBuffer.fill(std::numeric_limits<double>::infinity());
}

bool SoftwareRenderer::tryCreateVertexBuffer(int vertexCount, int componentsPerVertex, VertexBufferID *outID)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);

	if (!this->vertexBuffers.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate vertex buffer ID.");
		return false;
	}

	VertexBuffer &buffer = this->vertexBuffers.get(*outID);
	buffer.init(vertexCount, componentsPerVertex);
	return true;
}

bool SoftwareRenderer::tryCreateAttributeBuffer(int vertexCount, int componentsPerVertex, AttributeBufferID *outID)
{
	DebugAssert(vertexCount > 0);
	DebugAssert(componentsPerVertex >= 2);

	if (!this->attributeBuffers.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate attribute buffer ID.");
		return false;
	}

	AttributeBuffer &buffer = this->attributeBuffers.get(*outID);
	buffer.init(vertexCount, componentsPerVertex);
	return true;
}

bool SoftwareRenderer::tryCreateIndexBuffer(int indexCount, IndexBufferID *outID)
{
	DebugAssert(indexCount > 0);
	DebugAssert((indexCount % 3) == 0);

	if (!this->indexBuffers.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate index buffer ID.");
		return false;
	}

	IndexBuffer &buffer = this->indexBuffers.get(*outID);
	buffer.init(indexCount);
	return true;
}

void SoftwareRenderer::populateVertexBuffer(VertexBufferID id, const BufferView<const double> &vertices)
{
	VertexBuffer &buffer = this->vertexBuffers.get(id);
	const int srcCount = vertices.getCount();
	const int dstCount = buffer.vertices.getCount();
	if (srcCount != dstCount)
	{
		DebugLogError("Mismatched vertex buffer sizes for ID " + std::to_string(id) + ": " +
			std::to_string(srcCount) + " != " + std::to_string(dstCount));
		return;
	}

	const auto srcBegin = vertices.get();
	const auto srcEnd = srcBegin + srcCount;
	std::copy(srcBegin, srcEnd, buffer.vertices.get());
}

void SoftwareRenderer::populateAttributeBuffer(AttributeBufferID id, const BufferView<const double> &attributes)
{
	AttributeBuffer &buffer = this->attributeBuffers.get(id);
	const int srcCount = attributes.getCount();
	const int dstCount = buffer.attributes.getCount();
	if (srcCount != dstCount)
	{
		DebugLogError("Mismatched attribute buffer sizes for ID " + std::to_string(id) + ": " +
			std::to_string(srcCount) + " != " + std::to_string(dstCount));
		return;
	}

	const auto srcBegin = attributes.get();
	const auto srcEnd = srcBegin + srcCount;
	std::copy(srcBegin, srcEnd, buffer.attributes.get());
}

void SoftwareRenderer::populateIndexBuffer(IndexBufferID id, const BufferView<const int32_t> &indices)
{
	IndexBuffer &buffer = this->indexBuffers.get(id);
	const int srcCount = indices.getCount();
	const int dstCount = buffer.indices.getCount();
	if (srcCount != dstCount)
	{
		DebugLogError("Mismatched index buffer sizes for ID " + std::to_string(id) + ": " +
			std::to_string(srcCount) + " != " + std::to_string(dstCount));
		return;
	}

	const auto srcBegin = indices.get();
	const auto srcEnd = srcBegin + srcCount;
	std::copy(srcBegin, srcEnd, buffer.indices.get());
}

void SoftwareRenderer::freeVertexBuffer(VertexBufferID id)
{
	this->vertexBuffers.free(id);
}

void SoftwareRenderer::freeAttributeBuffer(AttributeBufferID id)
{
	this->attributeBuffers.free(id);
}

void SoftwareRenderer::freeIndexBuffer(IndexBufferID id)
{
	this->indexBuffers.free(id);
}

bool SoftwareRenderer::tryCreateObjectTexture(int width, int height, bool isPalette, ObjectTextureID *outID)
{
	if (!this->objectTextures.tryAlloc(outID))
	{
		DebugLogError("Couldn't allocate object texture ID.");
		return false;
	}

	ObjectTexture &texture = this->objectTextures.get(*outID);
	if (!isPalette)
	{
		texture.init8Bit(width, height);
		texture.texels.fill(0);
	}
	else
	{
		texture.initPalette(width * height);
		texture.paletteTexels.fill(0);
	}

	return true;
}

bool SoftwareRenderer::tryCreateObjectTexture(const TextureBuilder &textureBuilder, ObjectTextureID *outID)
{
	const int width = textureBuilder.getWidth();
	const int height = textureBuilder.getHeight();
	if (!this->tryCreateObjectTexture(width, height, false, outID))
	{
		DebugLogWarning("Couldn't create " + std::to_string(width) + "x" + std::to_string(height) + " object texture.");
		return false;
	}

	ObjectTexture &texture = this->objectTextures.get(*outID);
	uint8_t *dstTexels = texture.texels.get();

	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();
		const Buffer2D<uint8_t> &srcTexels = palettedTexture.texels;
		std::copy(srcTexels.get(), srcTexels.end(), dstTexels);
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		DebugLogWarning("True color texture (dimensions " + std::to_string(width) + "x" + std::to_string(height) + ") not supported.");
		texture.texels.fill(0);
		const TextureBuilder::TrueColorTexture &trueColorTexture = textureBuilder.getTrueColor();
		const Buffer2D<uint32_t> &srcTexels = trueColorTexture.texels;
		//std::transform(srcTexels.get(), srcTexels.end(), dstTexels, ...)
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}

	return true;
}

LockedTexture SoftwareRenderer::lockObjectTexture(ObjectTextureID id)
{
	ObjectTexture &texture = this->objectTextures.get(id);
	if (texture.texels.isValid())
	{
		return LockedTexture(texture.texels.get(), false);
	}
	else if (texture.paletteTexels.isValid())
	{
		return LockedTexture(texture.paletteTexels.get(), true);
	}
	else
	{
		DebugNotImplemented();
		return LockedTexture(nullptr, false);
	}
}

void SoftwareRenderer::unlockObjectTexture(ObjectTextureID id)
{
	// Do nothing; any writes are already in RAM.
	static_cast<void>(id);
}

void SoftwareRenderer::freeObjectTexture(ObjectTextureID id)
{
	this->objectTextures.free(id);
}

std::optional<Int2> SoftwareRenderer::tryGetObjectTextureDims(ObjectTextureID id) const
{
	const ObjectTexture &texture = this->objectTextures.get(id);
	return Int2(texture.texels.getWidth(), texture.texels.getHeight());
}

RendererSystem3D::ProfilerData SoftwareRenderer::getProfilerData() const
{
	const int renderWidth = this->depthBuffer.getWidth();
	const int renderHeight = this->depthBuffer.getHeight();

	const int threadCount = 1;
	const int drawCallCount = swGeometry::g_totalDrawCallCount;
	const int potentiallyVisTriangleCount = swGeometry::g_totalTriangleCount;
	const int visTriangleCount = swGeometry::g_visibleTriangleCount;
	const int visLightCount = 0;

	return ProfilerData(renderWidth, renderHeight, threadCount, drawCallCount, potentiallyVisTriangleCount,
		visTriangleCount, visLightCount);
}

void SoftwareRenderer::submitFrame(const RenderCamera &camera, const BufferView<const RenderDrawCall> &drawCalls,
	const RenderFrameSettings &settings, uint32_t *outputBuffer)
{
	const int frameBufferWidth = this->depthBuffer.getWidth();
	const int frameBufferHeight = this->depthBuffer.getHeight();
	BufferView2D<uint32_t> colorBufferView(outputBuffer, frameBufferWidth, frameBufferHeight);
	BufferView2D<double> depthBufferView(this->depthBuffer.get(), frameBufferWidth, frameBufferHeight);

	// Palette for 8-bit -> 32-bit color conversion.
	const ObjectTexture &paletteTexture = this->objectTextures.get(settings.paletteTextureID);

	// Light table for shading/transparency look-ups.
	const ObjectTexture &lightTableTexture = this->objectTextures.get(settings.lightTableTextureID);

	const uint32_t clearColor = Color::Black.toARGB();
	swRender::ClearFrameBuffers(clearColor, colorBufferView, depthBufferView);
	swRender::ClearTriangleDrawList();

	const swGeometry::ClippingPlanes clippingPlanes = swGeometry::MakeClippingPlanes(camera);

	const int drawCallCount = drawCalls.getCount();
	swGeometry::g_totalDrawCallCount = drawCallCount;

	for (int i = 0; i < drawCallCount; i++)
	{
		const RenderDrawCall &drawCall = drawCalls.get(i);
		const Double3 &meshPosition = drawCall.position;
		const Double3 &preScaleTranslation = drawCall.preScaleTranslation;
		const Matrix4d &rotationMatrix = drawCall.rotation;
		const Matrix4d &scaleMatrix = drawCall.scale;
		const VertexBuffer &vertexBuffer = this->vertexBuffers.get(drawCall.vertexBufferID);
		const AttributeBuffer &normalBuffer = this->attributeBuffers.get(drawCall.normalBufferID);
		const AttributeBuffer &texCoordBuffer = this->attributeBuffers.get(drawCall.texCoordBufferID);
		const IndexBuffer &indexBuffer = this->indexBuffers.get(drawCall.indexBufferID);
		const ObjectTextureID textureID0 = drawCall.textureIDs[0].has_value() ? *drawCall.textureIDs[0] : -1;
		const ObjectTextureID textureID1 = drawCall.textureIDs[1].has_value() ? *drawCall.textureIDs[1] : -1;
		const VertexShaderType vertexShaderType = drawCall.vertexShaderType;
		const swGeometry::TriangleDrawListIndices drawListIndices = swGeometry::ProcessMeshForRasterization(
			meshPosition, preScaleTranslation, rotationMatrix, scaleMatrix, vertexBuffer, normalBuffer, texCoordBuffer,
			indexBuffer, textureID0, textureID1, vertexShaderType, camera.worldPoint, clippingPlanes);

		const TextureSamplingType textureSamplingType = drawCall.textureSamplingType;
		const PixelShaderType pixelShaderType = drawCall.pixelShaderType;
		const double pixelShaderParam0 = drawCall.pixelShaderParam0;
		swRender::RasterizeTriangles(drawListIndices, textureSamplingType, pixelShaderType, pixelShaderParam0, 
			this->objectTextures, paletteTexture, lightTableTexture, camera, colorBufferView, depthBufferView);
	}
}

void SoftwareRenderer::present()
{
	// Do nothing for now, might change later.
}
