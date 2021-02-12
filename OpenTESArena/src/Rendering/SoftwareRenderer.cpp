#include <algorithm>
#include <cmath>
#include <emmintrin.h>
#include <immintrin.h>
#include <limits>
#include <smmintrin.h>

#include "ArenaRenderUtils.h"
#include "RendererUtils.h"
#include "RenderInitSettings.h"
#include "SoftwareRenderer.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Entities/EntityAnimationInstance.h"
#include "../Entities/EntityType.h"
#include "../Game/CardinalDirection.h"
#include "../Game/Options.h"
#include "../Interface/Surface.h"
#include "../Math/Constants.h"
#include "../Math/MathUtils.h"
#include "../Media/Color.h"
#include "../Media/Palette.h"
#include "../Utilities/Platform.h"
#include "../World/ArenaVoxelUtils.h"
#include "../World/ChunkUtils.h"
#include "../World/VoxelFacing2D.h"
#include "../World/VoxelGrid.h"
#include "../World/VoxelUtils.h"

#include "components/debug/Debug.h"

namespace
{
	// Hardcoded graphics options (will be loaded at runtime at some point).
	constexpr int TextureFilterMode = 0;
	constexpr bool LightContributionCap = true;

	constexpr double DEPTH_BUFFER_INFINITY = std::numeric_limits<double>::infinity();
}

void SoftwareRenderer::VoxelTexel::init(double r, double g, double b, double emission,
	bool transparent)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->emission = emission;
	this->transparent = transparent;
}

void SoftwareRenderer::FlatTexel::init(uint8_t value)
{
	this->value = value;
}

void SoftwareRenderer::SkyTexel::init(double r, double g, double b, double a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

void SoftwareRenderer::ChasmTexel::init(double r, double g, double b)
{
	this->r = r;
	this->g = g;
	this->b = b;
}

SoftwareRenderer::VoxelTexture::VoxelTexture()
{
	this->width = 0;
	this->height = 0;
}

void SoftwareRenderer::VoxelTexture::init(int width, int height, const uint8_t *srcTexels,
	const Palette &palette)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(width == height); // Must be square.
	DebugAssert(MathUtils::isPowerOf2(width)); // Must be power-of-two dimensions for mipmaps.
	DebugAssert(MathUtils::isPowerOf2(height));
	DebugAssert(srcTexels != nullptr);

	this->texels.resize(width * height);
	this->lightTexels.clear();
	this->width = width;
	this->height = height;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			const int index = x + (y * width);
			const uint8_t srcTexel = srcTexels[index];
			const Color &srcColor = palette[srcTexel];

			const Double4 dstColor = Double4::fromARGB(srcColor.toARGB());
			const double r = dstColor.x;
			const double g = dstColor.y;
			const double b = dstColor.z;
			constexpr double emission = 0.0;
			const bool transparent = dstColor.w == 0.0;

			VoxelTexel &dstTexel = this->texels[index];
			dstTexel.init(r, g, b, emission, transparent);

			// Check if the texel is used with night lights (yellow at night).
			if (srcTexel == ArenaRenderUtils::PALETTE_INDEX_NIGHT_LIGHT)
			{
				this->lightTexels.push_back(Int2(x, y));
			}
		}
	}
}

void SoftwareRenderer::VoxelTexture::setLightTexelsActive(bool active, const Palette &palette)
{
	constexpr int activePaletteIndex = ArenaRenderUtils::PALETTE_INDEX_NIGHT_LIGHT_ACTIVE;
	constexpr int inactivePaletteIndex = ArenaRenderUtils::PALETTE_INDEX_NIGHT_LIGHT_INACTIVE;
	const Color &activeColor = palette[activePaletteIndex];
	const Color &inactiveColor = palette[inactivePaletteIndex];

	// Change voxel texels based on whether it's night.
	const Double4 texelColor = Double4::fromARGB((active ? activeColor : inactiveColor).toARGB());
	const double texelEmission = active ? 1.0 : 0.0;

	for (const Int2 &lightTexel : this->lightTexels)
	{
		const int index = lightTexel.x + (lightTexel.y * this->width);

		DebugAssertIndex(this->texels, index);
		VoxelTexel &texel = this->texels[index];
		const double r = texelColor.x;
		const double g = texelColor.y;
		const double b = texelColor.z;
		const double emission = texelEmission;
		const bool transparent = texelColor.w == 0.0;
		texel.init(r, g, b, emission, transparent);
	}
}

SoftwareRenderer::FlatTexture::FlatTexture()
{
	this->width = 0;
	this->height = 0;
}

void SoftwareRenderer::FlatTexture::init(int width, int height, const uint8_t *srcTexels,
	bool flipped, bool reflective)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(srcTexels != nullptr);

	this->texels.resize(width * height);
	this->width = width;
	this->height = height;
	this->reflective = reflective;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			const int srcIndex = x + (y * width);
			const uint8_t srcTexel = srcTexels[srcIndex];

			// Texel order depends on whether the animation is flipped.
			const int dstIndex = !flipped ? srcIndex : (((width - 1) - x) + (y * width));
			FlatTexel &dstTexel = this->texels[dstIndex];
			dstTexel.init(srcTexel);
		}
	}
}

SoftwareRenderer::SkyTexture::SkyTexture()
{
	this->width = 0;
	this->height = 0;
}

void SoftwareRenderer::SkyTexture::init(int width, int height, const uint8_t *srcTexels,
	const Palette &palette)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(srcTexels != nullptr);

	this->texels.resize(width * height);
	this->width = width;
	this->height = height;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			const int index = x + (y * width);
			const uint8_t srcTexel = srcTexels[index];
			SkyTexel &dstTexel = this->texels[index];

			// Same as flat texels but for sky objects and without some hardcoded indices.
			if (ArenaRenderUtils::IsCloudTexel(srcTexel))
			{
				// Transparency for clouds.
				constexpr double r = 0.0;
				constexpr double g = 0.0;
				constexpr double b = 0.0;
				const double a = static_cast<double>(srcTexel) /
					static_cast<double>(ArenaRenderUtils::PALETTE_INDEX_SKY_LEVEL_DIVISOR);
				dstTexel.init(r, g, b, a);
			}
			else
			{
				// Color the texel normally.
				const Color &paletteColor = palette[srcTexel];
				const Double4 dstColor = Double4::fromARGB(paletteColor.toARGB());
				const double r = dstColor.x;
				const double g = dstColor.y;
				const double b = dstColor.z;
				const double a = dstColor.w;
				dstTexel.init(r, g, b, a);
			}
		}
	}
}

SoftwareRenderer::ChasmTexture::ChasmTexture()
{
	this->width = 0;
	this->height = 0;
}

void SoftwareRenderer::ChasmTexture::init(int width, int height, const uint8_t *srcTexels,
	const Palette &palette)
{
	DebugAssert(width > 0);
	DebugAssert(height > 0);
	DebugAssert(srcTexels != nullptr);

	this->texels.resize(width * height);
	this->width = width;
	this->height = height;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			const int index = x + (y * width);
			const uint8_t srcTexel = srcTexels[index];
			const Color &srcColor = palette[srcTexel];

			const Double4 dstColor = Double4::fromARGB(srcColor.toARGB());
			const double r = dstColor.x;
			const double g = dstColor.y;
			const double b = dstColor.z;

			ChasmTexel &dstTexel = this->texels[index];
			dstTexel.init(r, g, b);
		}
	}
}

SoftwareRenderer::VoxelTextureMapping::VoxelTextureMapping(TextureAssetReference &&textureAssetRef, int textureIndex)
	: textureAssetRef(std::move(textureAssetRef))
{
	this->textureIndex = textureIndex;
}

void SoftwareRenderer::VoxelTextures::addTexture(VoxelTexture &&texture, TextureAssetReference &&textureAssetRef)
{
	this->textures.emplace_back(std::move(texture));

	const int index = static_cast<int>(this->textures.size()) - 1;
	this->mappings.emplace_back(VoxelTextureMapping(std::move(textureAssetRef), index));
}

const SoftwareRenderer::VoxelTexture &SoftwareRenderer::VoxelTextures::getTexture(
	const TextureAssetReference &textureAssetRef) const
{
	int index = -1;
	for (int i = 0; i < static_cast<int>(this->mappings.size()); i++)
	{
		const VoxelTextureMapping &mapping = this->mappings[i];
		if (mapping.textureAssetRef == textureAssetRef)
		{
			index = mapping.textureIndex;
			break;
		}
	}

	DebugAssertIndex(this->textures, index);
	return this->textures[index];
}

void SoftwareRenderer::VoxelTextures::clear()
{
	this->textures.clear();
	this->mappings.clear();
}

bool SoftwareRenderer::FlatTextureGroup::isValidLookup(int stateID, int angleID, int textureID) const
{
	if (!DebugValidIndex(this->states, stateID))
	{
		DebugLogWarning("Invalid state ID \"" + std::to_string(stateID) +
			"\" (states: " + std::to_string(this->states.size()) + ").");
		return false;
	}

	const State &state = this->states[stateID];
	if (!DebugValidIndex(state, angleID))
	{
		DebugLogWarning("Invalid angle ID \"" + std::to_string(angleID) + "\" (state " +
			std::to_string(stateID) + ", angles: " + std::to_string(state.size()) + ").");
		return false;
	}

	const FlatTextureGroup::TextureList &textureList = state[angleID];
	if (!DebugValidIndex(textureList, textureID))
	{
		DebugLogWarning("Invalid texture ID \"" + std::to_string(textureID) + "\" (state " +
			std::to_string(stateID) + ", angle " + std::to_string(angleID) + ", textures: " +
			std::to_string(textureList.size()) + ").");
		return false;
	}

	return true;
}

const SoftwareRenderer::FlatTexture &SoftwareRenderer::FlatTextureGroup::getTexture(
	int stateID, int angleID, int textureID) const
{
	DebugAssert(this->isValidLookup(stateID, angleID, textureID));
	const State &state = this->states[stateID];
	const FlatTextureGroup::TextureList &textureList = state[angleID];
	const FlatTexture &texture = textureList[textureID];
	return texture;
}

void SoftwareRenderer::FlatTextureGroup::init(const EntityAnimationInstance &animInst)
{
	// Resize each state/keyframe buffer to fit all entity animation keyframes.
	const int stateCount = animInst.getStateCount();
	this->states.resize(stateCount);
	for (int stateIndex = 0; stateIndex < stateCount; stateIndex++)
	{
		const EntityAnimationInstance::State &animState = animInst.getState(stateIndex);
		const int keyframeListCount = animState.getKeyframeListCount();

		FlatTextureGroup::State &flatState = this->states[stateIndex];
		flatState.resize(keyframeListCount);
		for (int listIndex = 0; listIndex < keyframeListCount; listIndex++)
		{
			const EntityAnimationInstance::KeyframeList &animKeyframeList = animState.getKeyframeList(listIndex);
			const int keyframeCount = animKeyframeList.getKeyframeCount();

			FlatTextureGroup::TextureList &flatTextureList = flatState[listIndex];
			flatTextureList.resize(keyframeCount);
			for (FlatTexture &flatTexture : flatTextureList)
			{
				// Set texture to empty, to be initialized by caller next.
				flatTexture.width = 0;
				flatTexture.height = 0;
				flatTexture.texels.resize(0);
			}
		}
	}
}

void SoftwareRenderer::FlatTextureGroup::setTexture(int stateID, int angleID, int textureID,
	bool flipped, const TextureBuilder &textureBuilder, bool reflective)
{
	if (!this->isValidLookup(stateID, angleID, textureID))
	{
		DebugLogWarning("Invalid flat texture group look-up (" + std::to_string(stateID) +
			", " + std::to_string(angleID) + ", " + std::to_string(textureID) + ").");
		return;
	}

	FlatTextureGroup::State &state = this->states[stateID];
	FlatTextureGroup::TextureList &textureList = state[angleID];
	FlatTexture &texture = textureList[textureID];

	// @todo: figure out how the texture type should be handled in the long term. This renderer might
	// end up being strictly 8-bit.
	DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
	const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
	const Buffer2D<uint8_t> &srcTexels = srcTexture.texels;

	texture.init(srcTexels.getWidth(), srcTexels.getHeight(), srcTexels.get(), flipped, reflective);
}

SoftwareRenderer::Camera::Camera(const CoordDouble3 &eye, const VoxelDouble3 &direction,
	Degrees fovY, double aspect, double projectionModifier)
	: eye(eye), direction(direction)
{
	// Variations of eye position for certain voxel calculations.
	this->eyeVoxelReal = [&eye]()
	{
		const VoxelDouble3 voxel(
			std::floor(eye.point.x),
			std::floor(eye.point.y),
			std::floor(eye.point.z));
		return CoordDouble3(eye.chunk, voxel);
	}();

	this->eyeVoxel = [this]()
	{
		const VoxelInt3 voxel(
			static_cast<int>(this->eyeVoxelReal.point.x),
			static_cast<int>(this->eyeVoxelReal.point.y),
			static_cast<int>(this->eyeVoxelReal.point.z));
		return CoordInt3(this->eyeVoxelReal.chunk, voxel);
	}();

	// Camera axes. We trick the 2.5D ray caster into thinking the player is always looking
	// straight forward, but we use the Y component of the player's direction to offset 
	// projected coordinates via Y-shearing.
	const Double3 forwardXZ = Double3(direction.x, 0.0, direction.z).normalized();
	const Double3 rightXZ = forwardXZ.cross(Double3::UnitY).normalized();

	// Transformation matrix (model matrix isn't required because it's just the identity).
	this->transform = [&eye, &forwardXZ, &rightXZ, fovY, aspect, projectionModifier]()
	{
		// Global up vector, scaled by the projection modifier (i.e., to account for tall pixels).
		const Double3 up = Double3::UnitY * projectionModifier;

		// Convert chunk point to absolute point (relative to chunk 0, 0).
		// @todo: use eye point relative to the current chunk so all chunks' vertices can be relative to this.
		// - ideally there is no 'absolute point'.
		const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(eye);
		const Matrix4d view = Matrix4d::view(absoluteEye, forwardXZ, rightXZ, up);

		const Matrix4d projection = Matrix4d::perspective(fovY, aspect,
			SoftwareRenderer::NEAR_PLANE, SoftwareRenderer::FAR_PLANE);
		return projection * view;
	}();

	this->forwardX = forwardXZ.x;
	this->forwardZ = forwardXZ.z;
	this->rightX = rightXZ.x;
	this->rightZ = rightXZ.z;

	this->fovX = MathUtils::verticalFovToHorizontalFov(fovY, aspect);
	this->fovY = fovY;

	// Zoom of the camera, based on vertical field of view.
	this->zoom = MathUtils::verticalFovToZoom(fovY);
	this->aspect = aspect;

	// Forward and right modifiers, for interpolating 3D vectors across the screen and
	// so vertical FOV and aspect ratio are taken into consideration.
	this->forwardZoomedX = this->forwardX * this->zoom;
	this->forwardZoomedZ = this->forwardZ * this->zoom;
	this->rightAspectedX = this->rightX * this->aspect;
	this->rightAspectedZ = this->rightZ * this->aspect;

	// Left and right 2D vectors of the view frustum (at left and right edges of the screen).
	const NewDouble2 frustumLeft = NewDouble2(
		this->forwardZoomedX - this->rightAspectedX,
		this->forwardZoomedZ - this->rightAspectedZ).normalized();
	const NewDouble2 frustumRight = NewDouble2(
		this->forwardZoomedX + this->rightAspectedX,
		this->forwardZoomedZ + this->rightAspectedZ).normalized();
	this->frustumLeftX = frustumLeft.x;
	this->frustumLeftZ = frustumLeft.y;
	this->frustumRightX = frustumRight.x;
	this->frustumRightZ = frustumRight.y;

	// Vertical angle of the camera relative to the horizon.
	this->yAngleRadians = direction.getYAngleRadians();

	// Y-shearing is the distance that projected Y coordinates are translated by based on the 
	// player's 3D direction and field of view. First get the player's angle relative to the 
	// horizon, then get the tangent of that angle. The Y component of the player's direction
	// must be clamped less than 1 because 1 would imply they are looking straight up or down, 
	// which is impossible in 2.5D rendering (the vertical line segment of the view frustum 
	// would be infinitely high or low). The camera code should take care of the clamping for us.

	// Get the number of screen heights to translate all projected Y coordinates by, relative to
	// the current zoom. As a reference, this should be some value roughly between -1.0 and 1.0
	// for "acceptable skewing" at a vertical FOV of 90.0. If the camera is not clamped, this
	// could theoretically be between -infinity and infinity, but it would result in far too much
	// skewing.
	this->yShear = RendererUtils::getYShear(this->yAngleRadians, this->zoom);

	this->horizonProjY = [this]()
	{
		// Project a point directly in front of the player in the XZ plane.
		const Double3 horizonPoint = this->eye.point + Double3(this->direction.x, 0.0, this->direction.z);
		Double4 horizonProjPoint = this->transform * Double4(horizonPoint, 1.0);
		horizonProjPoint = horizonProjPoint / horizonProjPoint.w;
		return (0.50 + this->yShear) - (horizonProjPoint.y * 0.50);
	}();
}

Radians SoftwareRenderer::Camera::getXZAngleRadians() const
{
	return MathUtils::fullAtan2(-this->forwardX, -this->forwardZ);
}

int SoftwareRenderer::Camera::getAdjustedEyeVoxelY(double ceilingHeight) const
{
	return static_cast<int>(this->eye.point.y / ceilingHeight);
}

SoftwareRenderer::Ray::Ray(SNDouble dirX, WEDouble dirZ)
{
	this->dirX = dirX;
	this->dirZ = dirZ;
}

SoftwareRenderer::DrawRange::DrawRange(double yProjStart, double yProjEnd, int yStart, int yEnd)
{
	this->yProjStart = yProjStart;
	this->yProjEnd = yProjEnd;
	this->yStart = yStart;
	this->yEnd = yEnd;
}

SoftwareRenderer::OcclusionData::OcclusionData(int yMin, int yMax)
{
	this->yMin = yMin;
	this->yMax = yMax;
}

SoftwareRenderer::OcclusionData::OcclusionData()
	: OcclusionData(0, 0) { }

void SoftwareRenderer::OcclusionData::clipRange(int *yStart, int *yEnd) const
{
	const bool occluded = (*yEnd <= this->yMin) || (*yStart >= this->yMax);

	if (occluded)
	{
		// The drawing range is completely hidden.
		*yStart = *yEnd;
	}
	else
	{
		// Clip the drawing range.
		*yStart = std::max(*yStart, this->yMin);
		*yEnd = std::min(*yEnd, this->yMax);
	}
}

void SoftwareRenderer::OcclusionData::update(int yStart, int yEnd)
{
	// Slightly different than clipRange() because values just needs to be adjacent
	// rather than overlap.
	const bool canIncreaseMin = yStart <= this->yMin;
	const bool canDecreaseMax = yEnd >= this->yMax;

	// Determine how to update the occlusion ranges.
	if (canIncreaseMin && canDecreaseMax)
	{
		// The drawing range touches the top and bottom occlusion values, so the 
		// entire column is occluded.
		this->yMin = this->yMax;
	}
	else if (canIncreaseMin)
	{
		// Move the top of the window downward.
		this->yMin = std::max(yEnd, this->yMin);
	}
	else if (canDecreaseMax)
	{
		// Move the bottom of the window upward.
		this->yMax = std::min(yStart, this->yMax);
	}
}

SoftwareRenderer::ShadingInfo::ShadingInfo(const Palette &palette, const std::vector<Double3> &skyPalette,
	double daytimePercent, double latitude, double ambient, double fogDistance,
	double chasmAnimPercent, bool nightLightsAreActive, bool isExterior, bool playerHasLight)
{
	this->palette = palette;
	this->timeRotation = RendererUtils::getTimeOfDayRotation(daytimePercent);
	this->latitudeRotation = RendererUtils::getLatitudeRotation(latitude);
	this->nightLightsAreActive = nightLightsAreActive;

	BufferView<Double3> skyColorsView(this->skyColors.data(), static_cast<int>(this->skyColors.size()));
	RendererUtils::writeSkyColors(skyPalette, skyColorsView, daytimePercent);

	this->sunDirection = RendererUtils::getSunDirection(this->timeRotation, latitude);
	this->isExterior = isExterior;
	this->ambient = ambient;
	this->distantAmbient = RendererUtils::getDistantAmbientPercent(ambient);
	this->fogDistance = fogDistance;
	this->chasmAnimPercent = chasmAnimPercent;
	this->playerHasLight = playerHasLight;
}

const Double3 &SoftwareRenderer::ShadingInfo::getFogColor() const
{
	// The fog color is the same as the horizon color.
	return this->skyColors.front();
}

SoftwareRenderer::FrameView::FrameView(uint32_t *colorBuffer, double *depthBuffer, 
	int width, int height)
{
	this->colorBuffer = colorBuffer;
	this->depthBuffer = depthBuffer;
	this->width = width;
	this->height = height;
	this->widthReal = static_cast<double>(width);
	this->heightReal = static_cast<double>(height);
}

template <typename T>
SoftwareRenderer::DistantObject<T>::DistantObject(const T &obj, int textureIndex)
	: obj(obj)
{
	this->textureIndex = textureIndex;
}

SoftwareRenderer::DistantObjects::DistantObjects()
{
	this->sunTextureIndex = DistantObjects::NO_SUN;
}

void SoftwareRenderer::DistantObjects::init(const DistantSky &distantSky,
	std::vector<SkyTexture> &skyTextures, const Palette &palette, TextureManager &textureManager)
{
	DebugAssert(skyTextures.size() == 0);

	this->distantSky = &distantSky;
	this->textureManager = &textureManager;

	// Creates a render texture from the given 8-bit image ID, adds it to the sky textures list,
	// and returns its index in the sky textures list.
	auto addSkyTexture = [&skyTextures, &palette, &textureManager](TextureBuilderID textureBuilderID)
	{
		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(textureBuilderID);

		// @todo: decide how to handle texture builder format -- this renderer will probably
		// eventually only be 8-bit, but it is 32-bit for now.
		DebugAssert(textureBuilder.getType() == TextureBuilder::Type::Paletted);
		const TextureBuilder::PalettedTexture &srcTexture = textureBuilder.getPaletted();
		const Buffer2D<uint8_t> &srcTexels = srcTexture.texels;

		skyTextures.push_back(SkyTexture());
		SkyTexture &texture = skyTextures.back();
		texture.init(srcTexels.getWidth(), srcTexels.getHeight(), srcTexels.get(), palette);

		return static_cast<int>(skyTextures.size()) - 1;
	};

	// Creates a render texture with a single texel for small stars.
	auto addSmallStarTexture = [&skyTextures](uint32_t color)
	{
		skyTextures.push_back(SkyTexture());
		SkyTexture &texture = skyTextures.back();
		texture.texels = std::vector<SkyTexel>(1);
		texture.width = 1;
		texture.height = 1;

		// Small stars are never transparent in the original game; this is just using the
		// same storage representation as clouds which can have some transparencies.
		const Double4 srcColor = Double4::fromARGB(color);
		SkyTexel &dstTexel = texture.texels.front();
		dstTexel.r = srcColor.x;
		dstTexel.g = srcColor.y;
		dstTexel.b = srcColor.z;
		dstTexel.a = srcColor.w;

		return static_cast<int>(skyTextures.size()) - 1;
	};

	// Reverse iterate through each distant object type in the distant sky, creating associations
	// between the distant sky object and its render texture. Order of insertion matters.
	for (int i = distantSky.getLandObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::LandObject &landObject = distantSky.getLandObject(i);
		const int entryIndex = landObject.getTextureEntryIndex();
		const TextureAssetReference &textureAssetRef = distantSky.getTextureAssetRef(entryIndex);
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef.filename.c_str());
		if (!textureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
			continue;
		}

		const int textureIndex = addSkyTexture(*textureBuilderID);
		this->lands.push_back(DistantObject<DistantSky::LandObject>(landObject, textureIndex));
	}

	for (int i = distantSky.getAnimatedLandObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::AnimatedLandObject &animLandObject = distantSky.getAnimatedLandObject(i);
		const int setEntryIndex = animLandObject.getTextureSetEntryIndex();
		const std::string &setEntryFilename = distantSky.getTextureSetFilename(setEntryIndex);
		const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
			textureManager.tryGetTextureBuilderIDs(setEntryFilename.c_str());
		if (!textureBuilderIDs.has_value())
		{
			DebugLogError("Couldn't get texture builder IDs for \"" + setEntryFilename + "\".");
			continue;
		}

		const int setEntryCount = textureBuilderIDs->getCount();
		if (setEntryCount == 0)
		{
			DebugLogError("Need at least one animated land texture from \"" + setEntryFilename + "\".");
			continue;
		}

		// Add first texture to get the start index of the animated textures.
		const int textureIndex = addSkyTexture(textureBuilderIDs->getID(0));

		for (int j = 1; j < setEntryCount; j++)
		{
			addSkyTexture(textureBuilderIDs->getID(j));
		}

		this->animLands.push_back(DistantObject<DistantSky::AnimatedLandObject>(animLandObject, textureIndex));
	}

	for (int i = distantSky.getAirObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::AirObject &airObject = distantSky.getAirObject(i);
		const int entryIndex = airObject.getTextureEntryIndex();
		const TextureAssetReference &textureAssetRef = distantSky.getTextureAssetRef(entryIndex);
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef.filename.c_str());
		if (!textureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
			continue;
		}

		const int textureIndex = addSkyTexture(*textureBuilderID);
		this->airs.push_back(DistantObject<DistantSky::AirObject>(airObject, textureIndex));
	}

	for (int i = distantSky.getMoonObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::MoonObject &moonObject = distantSky.getMoonObject(i);
		const int entryIndex = moonObject.getTextureEntryIndex();
		const TextureAssetReference &textureAssetRef = distantSky.getTextureAssetRef(entryIndex);
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef.filename.c_str());
		if (!textureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
			continue;
		}

		const int textureIndex = addSkyTexture(*textureBuilderID);
		this->moons.push_back(DistantObject<DistantSky::MoonObject>(moonObject, textureIndex));
	}

	for (int i = distantSky.getStarObjectCount() - 1; i >= 0; i--)
	{
		const DistantSky::StarObject &starObject = distantSky.getStarObject(i);
		const int textureIndex = [&distantSky, &textureManager, &addSkyTexture, &addSmallStarTexture, &starObject]()
		{
			if (starObject.getType() == DistantSky::StarObject::Type::Small)
			{
				const DistantSky::StarObject::SmallStar &smallStar = starObject.getSmallStar();
				return addSmallStarTexture(smallStar.color);
			}
			else if (starObject.getType() == DistantSky::StarObject::Type::Large)
			{
				const DistantSky::StarObject::LargeStar &largeStar = starObject.getLargeStar();
				const int entryIndex = largeStar.entryIndex;
				const TextureAssetReference &textureAssetRef = distantSky.getTextureAssetRef(entryIndex);
				const std::optional<TextureBuilderID> textureBuilderID =
					textureManager.tryGetTextureBuilderID(textureAssetRef.filename.c_str());
				if (!textureBuilderID.has_value())
				{
					DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
				}

				return addSkyTexture(*textureBuilderID);
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(starObject.getType())));
			}
		}();

		this->stars.push_back(DistantObject<DistantSky::StarObject>(starObject, textureIndex));
	}

	if (distantSky.hasSun())
	{
		// Add the sun to the sky textures and assign its texture index.
		const int sunEntryIndex = distantSky.getSunEntryIndex();
		const TextureAssetReference &textureAssetRef = distantSky.getTextureAssetRef(sunEntryIndex);
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef.filename.c_str());
		if (!textureBuilderID.has_value())
		{
			DebugCrash("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
		}

		this->sunTextureIndex = addSkyTexture(*textureBuilderID);
	}
}

void SoftwareRenderer::DistantObjects::clear()
{
	this->lands.clear();
	this->animLands.clear();
	this->airs.clear();
	this->moons.clear();
	this->stars.clear();
	this->sunTextureIndex = DistantObjects::NO_SUN;
}

SoftwareRenderer::VisDistantObject::VisDistantObject(const SkyTexture &texture, DrawRange &&drawRange,
	double xProjStart, double xProjEnd, int xStart, int xEnd, bool emissive)
	: drawRange(std::move(drawRange))
{
	this->texture = &texture;
	this->xProjStart = xProjStart;
	this->xProjEnd = xProjEnd;
	this->xStart = xStart;
	this->xEnd = xEnd;
	this->emissive = emissive;
}

SoftwareRenderer::VisDistantObjects::VisDistantObjects()
{
	this->landStart = 0;
	this->landEnd = 0;
	this->animLandStart = 0;
	this->animLandEnd = 0;
	this->airStart = 0;
	this->airEnd = 0;
	this->moonStart = 0;
	this->moonEnd = 0;
	this->sunStart = 0;
	this->sunEnd = 0;
	this->starStart = 0;
	this->starEnd = 0;
}

void SoftwareRenderer::VisDistantObjects::clear()
{
	this->objs.clear();
	this->landStart = 0;
	this->landEnd = 0;
	this->animLandStart = 0;
	this->animLandEnd = 0;
	this->airStart = 0;
	this->airEnd = 0;
	this->moonStart = 0;
	this->moonEnd = 0;
	this->sunStart = 0;
	this->sunEnd = 0;
	this->starStart = 0;
	this->starEnd = 0;
}

void SoftwareRenderer::VisibleLight::init(const Double3 &position, double radius)
{
	this->position = position;
	this->radius = radius;
}

void SoftwareRenderer::LightVisibilityData::init(const Double3 &position, double radius,
	bool intersectsFrustum)
{
	this->position = position;
	this->radius = radius;
	this->intersectsFrustum = intersectsFrustum;
}

SoftwareRenderer::VisibleLightList::VisibleLightList()
{
	this->clear();
}

bool SoftwareRenderer::VisibleLightList::isFull() const
{
	return this->count == static_cast<int>(this->lightIDs.size());
}

void SoftwareRenderer::VisibleLightList::add(LightID lightID)
{
	DebugAssert(this->count < this->lightIDs.size());
	this->lightIDs[this->count] = lightID;
	this->count++;
}

void SoftwareRenderer::VisibleLightList::clear()
{
	this->count = 0;
}

void SoftwareRenderer::VisibleLightList::sortByNearest(const Double3 &point,
	const BufferView<const VisibleLight> &visLights)
{
	// @todo: can only do this if we know the lightID index when sorting.
	// Cache distance calculations for less redundant work.
	/*std::array<bool, std::tuple_size<decltype(this->lightIDs)>::value> validCacheDists;
	std::array<double, std::tuple_size<decltype(this->lightIDs)>::value> cachedDists;
	validCacheDists.fill(false);*/

	const auto startIter = this->lightIDs.begin();
	const auto endIter = startIter + this->count;

	std::sort(startIter, endIter, [&point, &visLights](LightID a, LightID b)
	{
		const VisibleLight &aLight = SoftwareRenderer::getVisibleLightByID(visLights, a);
		const VisibleLight &bLight = SoftwareRenderer::getVisibleLightByID(visLights, b);
		const double aDistSqr = (point - aLight.position).lengthSquared();
		const double bDistSqr = (point - bLight.position).lengthSquared();
		return aDistSqr < bDistSqr;
	});
}

void SoftwareRenderer::RenderThreadData::SkyGradient::init(double projectedYTop,
	double projectedYBottom, Buffer<Double3> &rowCache)
{
	this->threadsDone = 0;
	this->rowCache = &rowCache;
	this->projectedYTop = projectedYTop;
	this->projectedYBottom = projectedYBottom;
	this->shouldDrawStars = false;
}

void SoftwareRenderer::RenderThreadData::DistantSky::init(const VisDistantObjects &visDistantObjs,
	const std::vector<SkyTexture> &skyTextures)
{
	this->threadsDone = 0;
	this->visDistantObjs = &visDistantObjs;
	this->skyTextures = &skyTextures;
	this->doneVisTesting = false;
}

void SoftwareRenderer::RenderThreadData::Voxels::init(int chunkDistance, double ceilingHeight,
	const LevelData &levelData, const std::vector<VisibleLight> &visLights,
	const Buffer2D<VisibleLightList> &visLightLists, const VoxelTextures &voxelTextures,
	const ChasmTextureGroups &chasmTextureGroups, Buffer<OcclusionData> &occlusion)
{
	this->threadsDone = 0;
	this->chunkDistance = chunkDistance;
	this->ceilingHeight = ceilingHeight;
	this->levelData = &levelData;
	this->visLights = &visLights;
	this->visLightLists = &visLightLists;
	this->voxelTextures = &voxelTextures;
	this->chasmTextureGroups = &chasmTextureGroups;
	this->occlusion = &occlusion;
	this->doneLightVisTesting = false;
}

void SoftwareRenderer::RenderThreadData::Flats::init(const Double3 &flatNormal,
	const std::vector<VisibleFlat> &visibleFlats, const std::vector<VisibleLight> &visLights,
	const Buffer2D<VisibleLightList> &visLightLists, const FlatTextureGroups &flatTextureGroups)
{
	this->threadsDone = 0;
	this->flatNormal = &flatNormal;
	this->visibleFlats = &visibleFlats;
	this->visLights = &visLights;
	this->visLightLists = &visLightLists;
	this->flatTextureGroups = &flatTextureGroups;
	this->doneSorting = false;
}

SoftwareRenderer::RenderThreadData::RenderThreadData()
{
	// Make sure 'go' is initialized to false.
	this->totalThreads = 0;
	this->go = false;
	this->isDestructing = false;
	this->camera = nullptr;
	this->shadingInfo = nullptr;
	this->frame = nullptr;
}

void SoftwareRenderer::RenderThreadData::init(int totalThreads, const Camera &camera,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	this->totalThreads = totalThreads;
	this->camera = &camera;
	this->shadingInfo = &shadingInfo;
	this->frame = &frame;
	this->go = false;
	this->isDestructing = false;
}

SoftwareRenderer::SoftwareRenderer()
{
	// Initialize values to empty.
	this->width = 0;
	this->height = 0;
	this->renderThreadsMode = 0;
	this->fogDistance = 0.0;
}

SoftwareRenderer::~SoftwareRenderer()
{
	this->resetRenderThreads();
}

bool SoftwareRenderer::isInited() const
{
	// Frame buffer area must be positive.
	return (this->width > 0) && (this->height > 0);
}

SoftwareRenderer::ProfilerData SoftwareRenderer::getProfilerData() const
{
	// @todo: make this a member of SoftwareRenderer eventually when it is capturing more
	// information in render(), etc..
	return ProfilerData(this->width, this->height, this->renderThreads.getCount(),
		static_cast<int>(this->potentiallyVisibleFlats.size()), static_cast<int>(this->visibleFlats.size()),
		static_cast<int>(this->visibleLights.size()));
}

bool SoftwareRenderer::isValidEntityRenderID(EntityRenderID id) const
{
	return (id >= 0) && (id < static_cast<int>(this->flatTextureGroups.size()));
}

bool SoftwareRenderer::tryGetEntitySelectionData(const Double2 &uv, EntityRenderID entityRenderID,
	int animStateID, int animAngleID, int animKeyframeID, bool pixelPerfect, const Palette &palette,
	bool *outIsSelected) const
{
	// Branch depending on whether the selection request needs to include texture data.
	if (pixelPerfect)
	{
		// Get the texture list from the texture group at the given animation state and angle.
		DebugAssert(this->isValidEntityRenderID(entityRenderID));
		const FlatTextureGroup &textureGroup = this->flatTextureGroups[entityRenderID];
		const FlatTexture &texture = textureGroup.getTexture(animStateID, animAngleID, animKeyframeID);

		// Convert texture coordinates to a texture index. Don't need to clamp; just return
		// failure if it's out-of-bounds.
		const int textureX = static_cast<int>(uv.x * static_cast<double>(texture.width));
		const int textureY = static_cast<int>(uv.y * static_cast<double>(texture.height));

		if ((textureX < 0) || (textureX >= texture.width) ||
			(textureY < 0) || (textureY >= texture.height))
		{
			// Outside the texture.
			return false;
		}

		const int textureIndex = textureX + (textureY * texture.width);

		// Check if the texel is non-transparent.
		const FlatTexel &texel = texture.texels[textureIndex];
		const Color &texelColor = palette[texel.value];
		*outIsSelected = texelColor.a > 0;
		return true;
	}
	else
	{
		// If not pixel perfect, the entity's projected rectangle is hit if the texture coordinates
		// are valid.
		const bool withinEntity = (uv.x >= 0.0) && (uv.x <= 1.0) && (uv.y >= 0.0) && (uv.y <= 1.0);
		*outIsSelected = withinEntity;
		return true;
	}
}

Double3 SoftwareRenderer::screenPointToRay(double xPercent, double yPercent,
	const Double3 &cameraDirection, double fovY, double aspect) const
{
	// The basic components are the forward, up, and right vectors.
	const Double3 up = Double3::UnitY;
	const Double3 right = cameraDirection.cross(up).normalized();
	const Double3 forward = up.cross(right).normalized();

	// Building blocks of the ray direction. Up is reversed because y=0 is at the top
	// of the screen.
	const double rightPercent = ((xPercent * 2.0) - 1.0) * aspect;

	// Subtract y-shear from the Y percent because Y coordinates on-screen are reversed.
	const Radians yAngleRadians = cameraDirection.getYAngleRadians();
	const double zoom = MathUtils::verticalFovToZoom(fovY);
	const double yShear = RendererUtils::getYShear(yAngleRadians, zoom);
	const double upPercent = (((yPercent - yShear) * 2.0) - 1.0) / ArenaRenderUtils::TALL_PIXEL_RATIO;

	// Combine the various components to get the final vector
	const Double3 forwardComponent = forward * zoom;
	const Double3 rightComponent = right * rightPercent;
	const Double3 upComponent = up * upPercent;
	return (forwardComponent + rightComponent - upComponent).normalized();
}

void SoftwareRenderer::init(const RenderInitSettings &settings)
{
	// Initialize frame buffer.
	this->depthBuffer.init(settings.getWidth(), settings.getHeight());
	this->depthBuffer.fill(DEPTH_BUFFER_INFINITY);

	// Initialize occlusion columns.
	this->occlusion.init(settings.getWidth());
	this->occlusion.fill(OcclusionData(0, settings.getHeight()));

	// Initialize sky gradient cache.
	this->skyGradientRowCache.init(settings.getHeight());
	this->skyGradientRowCache.fill(Double3::Zero);

	// Initialize texture containers.
	this->voxelTextures = VoxelTextures();
	this->flatTextureGroups = FlatTextureGroups();

	this->width = settings.getWidth();
	this->height = settings.getHeight();
	this->renderThreadsMode = settings.getRenderThreadsMode();

	// Fog distance is zero by default.
	this->fogDistance = 0.0;

	// Initialize render threads.
	const int threadCount = RendererUtils::getRenderThreadsFromMode(settings.getRenderThreadsMode());
	this->initRenderThreads(settings.getWidth(), settings.getHeight(), threadCount);
}

void SoftwareRenderer::shutdown()
{
	// Don't need to free anything manually.
}

void SoftwareRenderer::setRenderThreadsMode(int mode)
{
	this->renderThreadsMode = mode;

	// Re-initialize render threads.
	const int threadCount = RendererUtils::getRenderThreadsFromMode(renderThreadsMode);
	this->initRenderThreads(this->width, this->height, threadCount);
}

EntityRenderID SoftwareRenderer::makeEntityRenderID()
{
	this->flatTextureGroups.push_back(FlatTextureGroup());
	return static_cast<EntityRenderID>(this->flatTextureGroups.size()) - 1;
}

void SoftwareRenderer::setFlatTextures(EntityRenderID entityRenderID,
	const EntityAnimationDefinition &animDef, const EntityAnimationInstance &animInst,
	bool isPuddle, TextureManager &textureManager)
{
	DebugAssert(this->isValidEntityRenderID(entityRenderID));
	FlatTextureGroup &flatTextureGroup = this->flatTextureGroups[entityRenderID];
	flatTextureGroup.init(animInst);

	for (int stateIndex = 0; stateIndex < animInst.getStateCount(); stateIndex++)
	{
		const EntityAnimationDefinition::State &defState = animDef.getState(stateIndex);
		const EntityAnimationInstance::State &instState = animInst.getState(stateIndex);
		const int keyframeListCount = defState.getKeyframeListCount();

		for (int keyframeListIndex = 0; keyframeListIndex < keyframeListCount; keyframeListIndex++)
		{
			const EntityAnimationDefinition::KeyframeList &defKeyframeList =
				defState.getKeyframeList(keyframeListIndex);
			const EntityAnimationInstance::KeyframeList &instKeyframeList =
				instState.getKeyframeList(keyframeListIndex);
			const int keyframeCount = defKeyframeList.getKeyframeCount();
			const bool flipped = defKeyframeList.isFlipped();

			for (int keyframeIndex = 0; keyframeIndex < keyframeCount; keyframeIndex++)
			{
				const EntityAnimationDefinition::Keyframe &defKeyframe =
					defKeyframeList.getKeyframe(keyframeIndex);
				const EntityAnimationInstance::Keyframe &instKeyframe =
					instKeyframeList.getKeyframe(keyframeIndex);
				const int stateID = stateIndex;
				const int angleID = keyframeListIndex;
				const int keyframeID = keyframeIndex;

				// Get the associated texture and write texture data.
				const TextureBuilder &textureBuilder =
					instKeyframe.getTextureBuilderHandle(defKeyframe, textureManager);
				const int textureID = keyframeID;
				flatTextureGroup.setTexture(stateID, angleID, textureID, flipped, textureBuilder, isPuddle);
			}
		}
	}
}

void SoftwareRenderer::setFogDistance(double fogDistance)
{
	this->fogDistance = fogDistance;
}

void SoftwareRenderer::setDistantSky(const DistantSky &distantSky, const Palette &palette,
	TextureManager &textureManager)
{
	// Clear old distant sky data.
	this->distantObjects.clear();
	this->skyTextures.clear();

	// Create distant objects and set the sky textures.
	this->distantObjects.init(distantSky, this->skyTextures, palette, textureManager);
}

void SoftwareRenderer::setSkyPalette(const uint32_t *colors, int count)
{
	this->skyPalette = std::vector<Double3>(count);

	for (size_t i = 0; i < this->skyPalette.size(); i++)
	{
		this->skyPalette[i] = Double3::fromRGB(colors[i]);
	}
}

void SoftwareRenderer::addChasmTexture(ArenaTypes::ChasmType chasmType,
	const uint8_t *colors, int width, int height, const Palette &palette)
{
	const int chasmID = RendererUtils::getChasmIdFromType(chasmType);

	auto iter = this->chasmTextureGroups.find(chasmID);
	if (iter == this->chasmTextureGroups.end())
	{
		iter = this->chasmTextureGroups.insert(std::make_pair(chasmID, ChasmTextureGroup())).first;
	}

	ChasmTextureGroup &textureGroup = iter->second;
	textureGroup.push_back(ChasmTexture());
	ChasmTexture &texture = textureGroup.back();
	texture.init(width, height, colors, palette);
}

void SoftwareRenderer::setNightLightsActive(bool active, const Palette &palette)
{
	// @todo: activate lights (don't worry about textures).

	for (VoxelTexture &voxelTexture : this->voxelTextures.textures)
	{
		voxelTexture.setLightTexelsActive(active, palette);
	}
}

void SoftwareRenderer::clearTexturesAndEntityRenderIDs()
{
	this->voxelTextures.clear();
	this->flatTextureGroups.clear();

	// Distant sky textures are cleared because the vector size is managed internally.
	this->skyTextures.clear();
	this->distantObjects.sunTextureIndex = SoftwareRenderer::DistantObjects::NO_SUN;

	this->chasmTextureGroups.clear();
}

void SoftwareRenderer::clearDistantSky()
{
	this->distantObjects.clear();
}

void SoftwareRenderer::resize(int width, int height)
{
	this->depthBuffer.init(width, height);
	this->depthBuffer.fill(DEPTH_BUFFER_INFINITY);

	this->occlusion.init(width);
	this->occlusion.fill(OcclusionData(0, height));

	this->skyGradientRowCache.init(height);
	this->skyGradientRowCache.fill(Double3::Zero);

	this->width = width;
	this->height = height;

	// Restart render threads with new dimensions.
	const int threadCount = RendererUtils::getRenderThreadsFromMode(this->renderThreadsMode);
	this->initRenderThreads(width, height, threadCount);
}

bool SoftwareRenderer::tryCreateVoxelTexture(const TextureAssetReference &textureAssetRef,
	TextureManager &textureManager)
{
	// @todo: protect against duplicate textures.

	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get voxel texture builder ID for \"" + textureAssetRef.filename + "\".");
		return false;
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		const TextureBuilder::PalettedTexture &palettedTexture = textureBuilder.getPaletted();

		// @todo: this method shouldn't care about the palette if it's 8-bit.
		const std::string &paletteFilename = ArenaPaletteName::Default;
		const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteFilename.c_str());
		if (!paletteID.has_value())
		{
			DebugCrash("Couldn't get palette ID for \"" + paletteFilename + "\".");
		}

		const Palette &palette = textureManager.getPaletteHandle(*paletteID);

		VoxelTexture voxelTexture;
		voxelTexture.init(textureBuilder.getWidth(), textureBuilder.getHeight(),
			palettedTexture.texels.get(), palette);

		this->voxelTextures.addTexture(std::move(voxelTexture), TextureAssetReference(textureAssetRef));
		return true;
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		// Not supported.
		return false;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}
}

bool SoftwareRenderer::tryCreateEntityTexture(const TextureAssetReference &textureAssetRef,
	TextureManager &textureManager)
{
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get entity texture builder ID for \"" + textureAssetRef.filename + "\".");
		return false;
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		DebugNotImplemented();
		return true;
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		// Not supported.
		return false;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}
}

bool SoftwareRenderer::tryCreateSkyTexture(const TextureAssetReference &textureAssetRef,
	TextureManager &textureManager)
{
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get sky texture builder ID for \"" + textureAssetRef.filename + "\".");
		return false;
	}

	const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
	const TextureBuilder::Type textureBuilderType = textureBuilder.getType();
	if (textureBuilderType == TextureBuilder::Type::Paletted)
	{
		DebugNotImplemented();
		return true;
	}
	else if (textureBuilderType == TextureBuilder::Type::TrueColor)
	{
		// Not supported.
		return false;
	}
	else
	{
		DebugUnhandledReturnMsg(bool, std::to_string(static_cast<int>(textureBuilderType)));
	}
}

void SoftwareRenderer::freeVoxelTexture(const TextureAssetReference &textureAssetRef)
{
	DebugNotImplemented();
}

void SoftwareRenderer::freeEntityTexture(const TextureAssetReference &textureAssetRef)
{
	DebugNotImplemented();
}

void SoftwareRenderer::freeSkyTexture(const TextureAssetReference &textureAssetRef)
{
	DebugNotImplemented();
}

void SoftwareRenderer::initRenderThreads(int width, int height, int threadCount)
{
	// If there are existing threads, reset them.
	if (this->renderThreads.getCount() > 0)
	{
		this->resetRenderThreads();
	}

	// If more or fewer threads are requested, re-allocate the render thread list.
	if (this->renderThreads.getCount() != threadCount)
	{
		this->renderThreads.init(threadCount);
	}

	// Block width and height are the approximate number of columns and rows per thread,
	// respectively.
	const double blockWidth = static_cast<double>(width) / static_cast<double>(threadCount);
	const double blockHeight = static_cast<double>(height) / static_cast<double>(threadCount);

	// Start thread loop for each render thread. Rounding is involved so the start and stop
	// coordinates are correct for all resolutions.
	for (int i = 0; i < this->renderThreads.getCount(); i++)
	{
		const int startX = static_cast<int>(std::round(static_cast<double>(i) * blockWidth));
		const int endX = static_cast<int>(std::round(static_cast<double>(i + 1) * blockWidth));
		const int startY = static_cast<int>(std::round(static_cast<double>(i) * blockHeight));
		const int endY = static_cast<int>(std::round(static_cast<double>(i + 1) * blockHeight));

		// Make sure the rounding is correct.
		DebugAssert(startX >= 0);
		DebugAssert(endX <= width);
		DebugAssert(startY >= 0);
		DebugAssert(endY <= height);

		this->renderThreads.set(i, std::thread(SoftwareRenderer::renderThreadLoop,
			std::ref(this->threadData), i, startX, endX, startY, endY));
	}
}

void SoftwareRenderer::resetRenderThreads()
{
	// Tell each render thread it needs to terminate.
	std::unique_lock<std::mutex> lk(this->threadData.mutex);
	this->threadData.go = true;
	this->threadData.isDestructing = true;
	lk.unlock();
	this->threadData.condVar.notify_all();

	for (int i = 0; i < this->renderThreads.getCount(); i++)
	{
		std::thread &thread = this->renderThreads.get(i);
		if (thread.joinable())
		{
			thread.join();
		}
	}

	// Set signal variables back to defaults, in case the render threads are used again.
	this->threadData.go = false;
	this->threadData.isDestructing = false;
}

void SoftwareRenderer::updateVisibleDistantObjects(const ShadingInfo &shadingInfo,
	const Camera &camera, const FrameView &frame)
{
	this->visDistantObjs.clear();

	// Directions forward and along the edges of the 2D frustum.
	const NewDouble2 forward(camera.forwardX, camera.forwardZ);
	const NewDouble2 frustumLeft(camera.frustumLeftX, camera.frustumLeftZ);
	const NewDouble2 frustumRight(camera.frustumRightX, camera.frustumRightZ);

	// Directions perpendicular to frustum vectors, for determining what points
	// are inside the frustum. Both directions point towards the inside.
	const NewDouble2 frustumLeftPerp = frustumLeft.rightPerp();
	const NewDouble2 frustumRightPerp = frustumRight.leftPerp();

	// Determines the vertical offset of the rendered object's origin on-screen. Most
	// objects have their origin at the bottom, but the sun has its origin at the top so
	// that when it's 6am or 6pm, its top edge will be at the horizon.
	enum class Orientation { Top, Bottom };

	// Lambda for checking if the given object properties make it appear on-screen, and if
	// so, adding it to the visible objects list.
	auto tryAddObject = [this, &camera, &frame, &forward, &frustumLeftPerp, &frustumRightPerp](
		const SkyTexture &texture, double xAngleRadians, double yAngleRadians, bool emissive,
		Orientation orientation)
	{
		const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);

		const double objWidth = static_cast<double>(texture.width) / DistantSky::IDENTITY_DIM;
		const double objHeight = static_cast<double>(texture.height) / DistantSky::IDENTITY_DIM;
		const double objHalfWidth = objWidth * 0.50;

		DrawRange drawRange = [yAngleRadians, orientation, &camera, &frame, &absoluteEye, objHeight]()
		{
			// Project the bottom first then add the object's height above it in screen-space
			// to get the top. This keeps objects from appearing squished the higher they are
			// in the sky. Don't need to worry about cases when the Y angle is at an extreme;
			// the start and end projections will both be off-screen (i.e., +inf or -inf).
			const Double3 objDirBottom = Double3(
				camera.forwardX,
				std::tan(yAngleRadians),
				camera.forwardZ).normalized();

			const Double3 objPointBottom = absoluteEye + objDirBottom;

			const double yProjEnd = RendererUtils::getProjectedY(
				objPointBottom, camera.transform, camera.yShear);
			const double yProjStart = yProjEnd - (objHeight * camera.zoom);

			const double yProjBias = (orientation == Orientation::Top) ?
				(yProjEnd - yProjStart) : 0.0;

			const double yProjScreenStart = (yProjStart + yProjBias) * frame.heightReal;
			const double yProjScreenEnd = (yProjEnd + yProjBias) * frame.heightReal;

			const int yStart = RendererUtils::getLowerBoundedPixel(yProjScreenStart, frame.height);
			const int yEnd = RendererUtils::getUpperBoundedPixel(yProjScreenEnd, frame.height);

			return DrawRange(yProjScreenStart, yProjScreenEnd, yStart, yEnd);
		}();

		// Calculate the object's position based on its midpoint like the original game.
		const Double3 objDir(
			-std::sin(xAngleRadians), // Negative for +X south/+Z west.
			0.0,
			-std::cos(xAngleRadians));

		// Create a point arbitrarily far away for the object's center in world space.
		const Double3 objPoint = absoluteEye + objDir;

		// Project the center point on-screen and get its projected X coordinate.
		const Double4 objProjPoint = camera.transform * Double4(objPoint, 1.0);
		const double xProjCenter = 0.50 + ((objProjPoint.x / objProjPoint.w) * 0.50);

		// Calculate the projected width of the object so we can get the left and right X
		// coordinates on-screen.
		const double objProjWidth = (objWidth * camera.zoom) /
			(camera.aspect * ArenaRenderUtils::TALL_PIXEL_RATIO);
		const double objProjHalfWidth = objProjWidth * 0.50;

		// Left and right coordinates of the object in screen space.
		const double xProjStart = xProjCenter - objProjHalfWidth;
		const double xProjEnd = xProjCenter + objProjHalfWidth;

		const NewDouble2 objDir2D(objDir.x, objDir.z);
		const bool onScreen = (objDir2D.dot(forward) > 0.0) && (xProjStart <= 1.0) && (xProjEnd >= 0.0);

		if (onScreen)
		{
			// Get the start and end X pixel coordinates.
			const int xDrawStart = RendererUtils::getLowerBoundedPixel(
				xProjStart * frame.widthReal, frame.width);
			const int xDrawEnd = RendererUtils::getUpperBoundedPixel(
				xProjEnd * frame.widthReal, frame.width);

			this->visDistantObjs.objs.push_back(VisDistantObject(
				texture, std::move(drawRange), xProjStart, xProjEnd, xDrawStart, xDrawEnd, emissive));
		}
	};

	// Iterate all distant objects and gather up the visible ones. Set the start
	// and end ranges for each object type to be used during rendering for
	// different types of shading.
	this->visDistantObjs.landStart = 0;

	for (const auto &land : this->distantObjects.lands)
	{
		const SkyTexture &texture = this->skyTextures.at(land.textureIndex);
		const Radians xAngleRadians = land.obj.getAngle();
		const Radians yAngleRadians = 0.0;
		const bool emissive = false;
		const Orientation orientation = Orientation::Bottom;

		tryAddObject(texture, xAngleRadians, yAngleRadians, emissive, orientation);
	}

	this->visDistantObjs.landEnd = static_cast<int>(this->visDistantObjs.objs.size());
	this->visDistantObjs.animLandStart = this->visDistantObjs.landEnd;

	for (const auto &animLand : this->distantObjects.animLands)
	{
		const DistantSky::AnimatedLandObject &animLandObj = animLand.obj;

		// Need to see how many frames the animated land has.
		const std::string &animLandFilename =
			this->distantObjects.distantSky->getTextureSetFilename(animLandObj.getTextureSetEntryIndex());
		const std::optional<TextureBuilderIdGroup> textureBuilderIDs =
			this->distantObjects.textureManager->tryGetTextureBuilderIDs(animLandFilename.c_str());
		if (!textureBuilderIDs.has_value())
		{
			DebugLogError("Couldn't get texture builder IDs for \"" + animLandFilename + "\".");
			continue;
		}

		const int animTextureCount = textureBuilderIDs->getCount();
		const double animPercent = animLandObj.getAnimPercent();
		const int curAnimTextureIndex = std::clamp(
			static_cast<int>(static_cast<double>(animTextureCount) * animPercent), 0, animTextureCount - 1);

		const int skyTextureIndex = DebugMakeIndex(this->skyTextures, animLand.textureIndex + curAnimTextureIndex);
		const SkyTexture &texture = this->skyTextures[skyTextureIndex];
		const Radians xAngleRadians = animLand.obj.getAngle();
		const Radians yAngleRadians = 0.0;
		const bool emissive = true;
		const Orientation orientation = Orientation::Bottom;

		tryAddObject(texture, xAngleRadians, yAngleRadians, emissive, orientation);
	}

	this->visDistantObjs.animLandEnd = static_cast<int>(this->visDistantObjs.objs.size());
	this->visDistantObjs.airStart = this->visDistantObjs.animLandEnd;

	for (const auto &air : this->distantObjects.airs)
	{
		const SkyTexture &texture = skyTextures.at(air.textureIndex);
		const Radians xAngleRadians = air.obj.getAngle();
		const Radians yAngleRadians = [&air]()
		{
			// 0 is at horizon, 1 is at top of distant cloud height limit.
			const double gradientPercent = air.obj.getHeight();
			return gradientPercent * (SoftwareRenderer::DISTANT_CLOUDS_MAX_ANGLE * Constants::DegToRad);
		}();

		const bool emissive = false;
		const Orientation orientation = Orientation::Bottom;

		tryAddObject(texture, xAngleRadians, yAngleRadians, emissive, orientation);
	}

	this->visDistantObjs.airEnd = static_cast<int>(this->visDistantObjs.objs.size());
	this->visDistantObjs.moonStart = this->visDistantObjs.airEnd;

	// Objects in space have their position modified by latitude and time of day.
	// My quaternions are broken or something, so use matrix multiplication instead.
	const Matrix4d &timeRotation = shadingInfo.timeRotation;
	const Matrix4d &latitudeRotation = shadingInfo.latitudeRotation;

	auto getSpaceCorrectedAngles = [&timeRotation, &latitudeRotation](Radians xAngleRadians,
		Radians yAngleRadians, Radians &newXAngleRadians, Radians &newYAngleRadians)
	{
		// Direction towards the space object.
		const Double3 direction = Double3(
			-std::sin(xAngleRadians), // Negative for +X south/+Z west.
			std::tan(yAngleRadians),
			-std::cos(xAngleRadians)).normalized();

		// Rotate the direction based on latitude and time of day.
		const Double4 dir = latitudeRotation * (timeRotation * Double4(direction, 0.0));

		// Don't negate for +X south/+Z west, they are negated when added to the draw list.
		newXAngleRadians = std::atan2(dir.x, dir.z);
		newYAngleRadians = std::asin(dir.y);
	};

	for (const auto &moon : this->distantObjects.moons)
	{
		const SkyTexture &texture = skyTextures.at(moon.textureIndex);

		// These moon directions are roughly correct, based on the original game.
		const Double3 direction = [&moon]()
		{
			const DistantSky::MoonObject::Type type = moon.obj.getType();

			double bonusLatitude;
			const Double3 baseDir = [type, &bonusLatitude]()
			{
				if (type == DistantSky::MoonObject::Type::First)
				{
					bonusLatitude = 15.0 / 100.0;
					return Double3(0.0, -57536.0, 0.0).normalized();
				}
				else if (type == DistantSky::MoonObject::Type::Second)
				{
					bonusLatitude = 30.0 / 100.0;
					return Double3(-3000.0, -53536.0, 0.0).normalized();
				}
				else
				{
					DebugUnhandledReturnMsg(Double3, std::to_string(static_cast<int>(type)));
				}
			}();

			// The moon's position in the sky is modified by its current phase.
			const double phaseModifier = moon.obj.getPhasePercent() + bonusLatitude;
			const Matrix4d moonRotation = RendererUtils::getLatitudeRotation(phaseModifier);
			const Double4 dir = moonRotation * Double4(baseDir, 0.0);
			return Double3(-dir.x, dir.y, -dir.z).normalized(); // Negative for +X south/+Z west.
		}();

		const Radians xAngleRadians = MathUtils::fullAtan2(-direction.x, -direction.z);
		const Radians yAngleRadians = direction.getYAngleRadians();
		const bool emissive = true;
		const Orientation orientation = Orientation::Top;

		// Modify angle based on latitude and time of day.
		Radians newXAngleRadians, newYAngleRadians;
		getSpaceCorrectedAngles(xAngleRadians, yAngleRadians, newXAngleRadians, newYAngleRadians);

		tryAddObject(texture, newXAngleRadians, newYAngleRadians, emissive, orientation);
	}

	this->visDistantObjs.moonEnd = static_cast<int>(this->visDistantObjs.objs.size());
	this->visDistantObjs.sunStart = this->visDistantObjs.moonEnd;

	// Try to add the sun to the visible distant objects.
	if (this->distantObjects.sunTextureIndex != SoftwareRenderer::DistantObjects::NO_SUN)
	{
		const SkyTexture &sunTexture = this->skyTextures.at(this->distantObjects.sunTextureIndex);

		// The sun direction is already corrected for latitude and time of day since the same
		// variable is reused with shading.
		const Double3 &sunDirection = shadingInfo.sunDirection;
		const Radians sunXAngleRadians = MathUtils::fullAtan2(-sunDirection.x, -sunDirection.z);

		// When the sun is directly above or below, it might cause the X angle to be undefined.
		// We want to filter this out before we try projecting it on-screen.
		if (std::isfinite(sunXAngleRadians))
		{
			const Radians sunYAngleRadians = sunDirection.getYAngleRadians();
			const bool sunEmissive = true;
			const Orientation sunOrientation = Orientation::Top;

			tryAddObject(sunTexture, sunXAngleRadians, sunYAngleRadians, sunEmissive, sunOrientation);
		}
	}

	this->visDistantObjs.sunEnd = static_cast<int>(this->visDistantObjs.objs.size());
	this->visDistantObjs.starStart = this->visDistantObjs.sunEnd;

	for (const auto &star : this->distantObjects.stars)
	{
		const SkyTexture &texture = skyTextures.at(star.textureIndex);

		const Double3 &direction = star.obj.getDirection();
		const Radians xAngleRadians = MathUtils::fullAtan2(-direction.x, -direction.z);
		const Radians yAngleRadians = direction.getYAngleRadians();
		const bool emissive = true;
		const Orientation orientation = Orientation::Bottom;

		// Modify angle based on latitude and time of day.
		Radians newXAngleRadians, newYAngleRadians;
		getSpaceCorrectedAngles(xAngleRadians, yAngleRadians, newXAngleRadians, newYAngleRadians);

		tryAddObject(texture, newXAngleRadians, newYAngleRadians, emissive, orientation);
	}

	this->visDistantObjs.starEnd = static_cast<int>(this->visDistantObjs.objs.size());
}

void SoftwareRenderer::updatePotentiallyVisibleFlats(const Camera &camera,
	SNInt gridWidth, WEInt gridDepth, int chunkDistance, const EntityManager &entityManager,
	std::vector<const Entity*> *outPotentiallyVisFlats, int *outEntityCount)
{
	const ChunkInt2 &cameraChunk = camera.eye.chunk;

	// Get the min and max chunk coordinates to loop over.
	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(cameraChunk, chunkDistance, &minChunk, &maxChunk);

	// Number of potentially visible chunks along each axis (i.e. 3x3).
	SNInt potentiallyVisChunkCountX;
	WEInt potentiallyVisChunkCountZ;
	ChunkUtils::getPotentiallyVisibleChunkCounts(chunkDistance,
		&potentiallyVisChunkCountX, &potentiallyVisChunkCountZ);

	auto getChunkPotentiallyVisFlatCount = [&entityManager](SNInt chunkX, WEInt chunkZ)
	{
		return entityManager.getTotalCountInChunk(ChunkInt2(chunkX, chunkZ));
	};

	auto getTotalPotentiallyVisFlatCount = [](const BufferView2D<const int> &chunkPotentiallyVisFlatCounts)
	{
		int count = 0;
		for (WEInt z = 0; z < chunkPotentiallyVisFlatCounts.getHeight(); z++)
		{
			for (SNInt x = 0; x < chunkPotentiallyVisFlatCounts.getWidth(); x++)
			{
				count += chunkPotentiallyVisFlatCounts.get(x, z);
			}
		}

		return count;
	};

	// Get potentially visible flat counts for each chunk.
	Buffer2D<int> chunkPotentiallyVisFlatCounts(potentiallyVisChunkCountX, potentiallyVisChunkCountZ);
	for (WEInt z = 0; z < chunkPotentiallyVisFlatCounts.getHeight(); z++)
	{
		for (SNInt x = 0; x < chunkPotentiallyVisFlatCounts.getWidth(); x++)
		{
			const SNInt chunkX = minChunk.x + x;
			const WEInt chunkZ = minChunk.y + z;
			const int count = getChunkPotentiallyVisFlatCount(chunkX, chunkZ);
			chunkPotentiallyVisFlatCounts.set(x, z, count);
		}
	}

	// Total potentially visible flat count (in the chunks surrounding the player).
	const int potentiallyVisFlatCount = getTotalPotentiallyVisFlatCount(BufferView2D<const int>(
		chunkPotentiallyVisFlatCounts.get(), chunkPotentiallyVisFlatCounts.getWidth(),
		chunkPotentiallyVisFlatCounts.getHeight()));

	auto addPotentiallyVisFlatsInChunk = [&entityManager, outPotentiallyVisFlats, &minChunk,
		&chunkPotentiallyVisFlatCounts](SNInt chunkX, WEInt chunkZ, int insertIndex)
	{
		const Entity **entitiesPtr = outPotentiallyVisFlats->data() + insertIndex;
		const SNInt visChunkX = chunkX - minChunk.x;
		const WEInt visChunkZ = chunkZ - minChunk.y;
		const int count = chunkPotentiallyVisFlatCounts.get(visChunkX, visChunkZ);
		const int writtenCount = entityManager.getTotalEntitiesInChunk(
			ChunkInt2(chunkX, chunkZ), entitiesPtr, count);
		DebugAssert(writtenCount <= count);
	};

	outPotentiallyVisFlats->resize(potentiallyVisFlatCount);

	int potentiallyVisFlatInsertIndex = 0;
	for (WEInt z = 0; z < potentiallyVisChunkCountZ; z++)
	{
		for (SNInt x = 0; x < potentiallyVisChunkCountX; x++)
		{
			const int chunkPotentiallyVisFlatCount = chunkPotentiallyVisFlatCounts.get(x, z);
			const SNInt chunkX = minChunk.x + x;
			const WEInt chunkZ = minChunk.y + z;
			addPotentiallyVisFlatsInChunk(chunkX, chunkZ, potentiallyVisFlatInsertIndex);
			potentiallyVisFlatInsertIndex += chunkPotentiallyVisFlatCount;
		}
	}

	*outEntityCount = potentiallyVisFlatInsertIndex;
}

void SoftwareRenderer::updateVisibleFlats(const Camera &camera, const ShadingInfo &shadingInfo,
	int chunkDistance, double ceilingHeight, const VoxelGrid &voxelGrid,
	const EntityManager &entityManager, const EntityDefinitionLibrary &entityDefLibrary)
{
	this->visibleFlats.clear();
	this->visibleLights.clear();

	// Update potentially visible flats so this method knows what to work with.
	int potentiallyVisFlatCount;
	SoftwareRenderer::updatePotentiallyVisibleFlats(camera, voxelGrid.getWidth(), voxelGrid.getDepth(),
		chunkDistance, entityManager, &this->potentiallyVisibleFlats, &potentiallyVisFlatCount);

	// Each flat shares the same axes. The forward direction always faces opposite to 
	// the camera direction.
	const Double3 flatForward = Double3(-camera.forwardX, 0.0, -camera.forwardZ).normalized();
	const Double3 flatUp = Double3::UnitY;
	const Double3 flatRight = flatForward.cross(flatUp).normalized();

	const CoordDouble2 eyeXZ(
		camera.eye.chunk,
		VoxelDouble2(camera.eye.point.x, camera.eye.point.z));
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewDouble2 absoluteEyeXZ(absoluteEye.x, absoluteEye.z);
	const NewDouble2 cameraDir(camera.forwardX, camera.forwardZ);

	if (shadingInfo.playerHasLight)
	{
		// Add player light.
		VisibleLight playerVisLight;
		playerVisLight.init(absoluteEye, 5.0);
		this->visibleLights.emplace_back(std::move(playerVisLight));
	}

	// Potentially visible flat determination algorithm, given the current camera.
	// Also calculates visible lights.
	for (int i = 0; i < potentiallyVisFlatCount; i++)
	{
		const Entity *entity = this->potentiallyVisibleFlats[i];

		// Entities can currently be null because of EntityGroup implementation details.
		if (entity == nullptr)
		{
			continue;
		}

		const EntityDefinition &entityDef = entityManager.getEntityDef(
			entity->getDefinitionID(), entityDefLibrary);

		EntityManager::EntityVisibilityData visData;
		entityManager.getEntityVisibilityData(*entity, eyeXZ, ceilingHeight, voxelGrid,
			entityDefLibrary, visData);

		// Get entity animation state to determine render properties.
		const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
		const EntityAnimationDefinition::State &animDefState = animDef.getState(visData.stateIndex);
		const EntityAnimationDefinition::KeyframeList &animDefKeyframeList =
			animDefState.getKeyframeList(visData.angleIndex);
		const EntityAnimationDefinition::Keyframe &animDefKeyframe =
			animDefKeyframeList.getKeyframe(visData.keyframeIndex);

		const double flatWidth = animDefKeyframe.getWidth();
		const double flatHeight = animDefKeyframe.getHeight();
		const double flatHalfWidth = flatWidth * 0.50;

		// See if the entity is a light.
		int lightIntensity;
		if (!EntityUtils::tryGetLightIntensity(entityDef, &lightIntensity))
		{
			constexpr int streetLightIntensity = 4;
			const bool isActiveStreetLight = ((entityDef.getType() == EntityDefinition::Type::Doodad) &&
				entityDef.getDoodad().streetlight) && shadingInfo.nightLightsAreActive;
			lightIntensity = isActiveStreetLight ? streetLightIntensity : 0;
		}

		const NewDouble3 absoluteFlatPosition = VoxelUtils::coordToNewPoint(visData.flatPosition);
		const bool isLight = lightIntensity > 0;
		if (isLight)
		{
			// See if the light is visible.
			SoftwareRenderer::LightVisibilityData lightVisData;
			SoftwareRenderer::getLightVisibilityData(absoluteFlatPosition, flatHeight,
				lightIntensity, absoluteEyeXZ, cameraDir, camera.fovX, fogDistance, &lightVisData);

			if (lightVisData.intersectsFrustum)
			{
				// Add a new visible light.
				VisibleLight visLight;
				visLight.init(lightVisData.position, lightVisData.radius);
				this->visibleLights.push_back(std::move(visLight));
			}
		}

		const NewDouble2 absoluteFlatPositionXZ(absoluteFlatPosition.x, absoluteFlatPosition.z);

		// Check if the flat is somewhere in front of the camera.
		const NewDouble2 flatEyeDiff = absoluteFlatPositionXZ - absoluteEyeXZ;
		const double flatEyeDiffLen = flatEyeDiff.length();
		const NewDouble2 flatEyeDir = flatEyeDiff / flatEyeDiffLen;
		const bool inFrontOfCamera = cameraDir.dot(flatEyeDir) > 0.0;

		// Check if the flat is within the fog distance. Treat the flat as a cylinder and
		// see if it's inside the fog distance circle centered on the player. Can't use
		// distance squared here because a^2 - b^2 does not equal (a - b)^2.
		const double flatRadius = flatHalfWidth;
		const double flatEyeCylinderDist = flatEyeDiffLen - flatRadius;
		const bool inFogDistance = flatEyeCylinderDist < fogDistance;

		if (inFrontOfCamera && inFogDistance)
		{
			// Scaled axes based on flat dimensions.
			const Double3 flatRightScaled = flatRight * flatHalfWidth;
			const Double3 flatUpScaled = flatUp * flatHeight;

			// Determine if the flat is potentially visible to the camera.
			VisibleFlat visFlat;
			visFlat.entityRenderID = entity->getRenderID();
			visFlat.animStateID = visData.stateIndex;
			visFlat.animAngleID = visData.angleIndex;
			visFlat.animTextureID = visData.keyframeIndex;

			// Calculate each corner of the flat in world space.
			visFlat.bottomLeft = absoluteFlatPosition + flatRightScaled;
			visFlat.bottomRight = absoluteFlatPosition - flatRightScaled;
			visFlat.topLeft = visFlat.bottomLeft + flatUpScaled;
			visFlat.topRight = visFlat.bottomRight + flatUpScaled;

			// Add palette override if it is a citizen entity.
			const EntityAnimationInstance &animInst = entity->getAnimInstance();
			const EntityAnimationInstance::CitizenParams *citizenParams = animInst.getCitizenParams();
			visFlat.overridePalette = (citizenParams != nullptr) ? &citizenParams->palette : nullptr;

			// Now project two of the flat's opposing corner points into camera space.
			// The Z value is used with flat sorting (not rendering), and the X and Y values 
			// are used to find where the flat is on-screen.
			Double4 projStart = camera.transform * Double4(visFlat.topLeft, 1.0);
			Double4 projEnd = camera.transform * Double4(visFlat.bottomRight, 1.0);

			// Normalize coordinates.
			projStart = projStart / projStart.w;
			projEnd = projEnd / projEnd.w;

			// Assign each screen value to the flat frame data.
			visFlat.startX = 0.50 + (projStart.x * 0.50);
			visFlat.endX = 0.50 + (projEnd.x * 0.50);
			visFlat.startY = (0.50 + camera.yShear) - (projStart.y * 0.50);
			visFlat.endY = (0.50 + camera.yShear) - (projEnd.y * 0.50);
			visFlat.z = projStart.z;

			// Check that the projected values are within view and are inside the near
			// and far clip planes.
			const bool inScreenX = (visFlat.startX < 1.0) && (visFlat.endX > 0.0);
			const bool inScreenY = (visFlat.startY < 1.0) && (visFlat.endY > 0.0);
			const bool inPlanes = (visFlat.z >= SoftwareRenderer::NEAR_PLANE) &&
				(visFlat.z <= SoftwareRenderer::FAR_PLANE);

			if (inScreenX && inScreenY && inPlanes)
			{
				// Add the flat data to the draw list.
				this->visibleFlats.push_back(std::move(visFlat));
			}
		}
	}

	// Sort the visible flats farthest to nearest (relevant for transparencies).
	std::sort(this->visibleFlats.begin(), this->visibleFlats.end(),
		[](const VisibleFlat &a, const VisibleFlat &b) { return a.z > b.z; });
}

void SoftwareRenderer::updateVisibleLightLists(const Camera &camera, int chunkDistance,
	double ceilingHeight, const VoxelGrid &voxelGrid)
{
	// Visible light lists are relative to the potentially visible chunks.
	const ChunkInt2 &cameraChunk = camera.eye.chunk;

	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(cameraChunk, chunkDistance, &minChunk, &maxChunk);

	// Get the closest-to-origin voxel in the potentially visible chunks so we can do some
	// relative chunk calculations.
	const NewInt2 minAbsoluteChunkVoxel = VoxelUtils::chunkVoxelToNewVoxel(minChunk, VoxelInt2(0, 0));

	SNInt potentiallyVisChunkCountX;
	WEInt potentiallyVisChunkCountZ;
	ChunkUtils::getPotentiallyVisibleChunkCounts(chunkDistance,
		&potentiallyVisChunkCountX, &potentiallyVisChunkCountZ);

	const SNInt visLightListVoxelCountX = potentiallyVisChunkCountX * ChunkUtils::CHUNK_DIM;
	const WEInt visLightListVoxelCountZ = potentiallyVisChunkCountZ * ChunkUtils::CHUNK_DIM;

	if (!this->visLightLists.isValid() ||
		(this->visLightLists.getWidth() != visLightListVoxelCountX) ||
		(this->visLightLists.getHeight() != visLightListVoxelCountZ))
	{
		this->visLightLists.init(visLightListVoxelCountX, visLightListVoxelCountZ);
	}

	// Clear all potentially visible light lists.
	for (WEInt z = 0; z < this->visLightLists.getHeight(); z++)
	{
		for (SNInt x = 0; x < this->visLightLists.getWidth(); x++)
		{
			VisibleLightList &visLightList = this->visLightLists.get(x, z);
			visLightList.clear();
		}
	}

	// Populate potentially visible light lists based on visible lights.
	for (size_t i = 0; i < this->visibleLights.size(); i++)
	{
		// Iterate over all voxels columns touched by the light.
		const VisibleLight &visLight = this->visibleLights[i];
		const VisibleLightList::LightID visLightID = static_cast<VisibleLightList::LightID>(i);

		// Bounding box around the light's reach in the XZ plane.
		const NewInt2 visLightMin(
			static_cast<SNInt>(std::floor(visLight.position.x - visLight.radius)),
			static_cast<WEInt>(std::floor(visLight.position.z - visLight.radius)));
		const NewInt2 visLightMax(
			static_cast<SNInt>(std::ceil(visLight.position.x + visLight.radius)),
			static_cast<WEInt>(std::ceil(visLight.position.z + visLight.radius)));

		// Since these are in a different coordinate system, can't rely on min < max.
		const NewInt2 visLightAbsoluteChunkVoxelA = visLightMin;
		const NewInt2 visLightAbsoluteChunkVoxelB = visLightMax;

		// Get chunk voxel coordinates relative to potentially visible chunks.
		const NewInt2 relativeChunkVoxelA = visLightAbsoluteChunkVoxelA - minAbsoluteChunkVoxel;
		const NewInt2 relativeChunkVoxelB = visLightAbsoluteChunkVoxelB - minAbsoluteChunkVoxel;

		// Have to rely on delta between A and B instead of min/max due to coordinate system transform.
		const Int2 relativeChunkVoxelDeltaStep(
			((relativeChunkVoxelB.x - relativeChunkVoxelA.x) > 0) ? 1 : -1,
			((relativeChunkVoxelB.y - relativeChunkVoxelA.y) > 0) ? 1 : -1);

		for (WEInt z = relativeChunkVoxelA.y; z != relativeChunkVoxelB.y; z += relativeChunkVoxelDeltaStep.y)
		{
			for (SNInt x = relativeChunkVoxelA.x; x != relativeChunkVoxelB.x; x += relativeChunkVoxelDeltaStep.x)
			{
				const bool coordIsValid = (x >= 0) && (x < visLightListVoxelCountX) &&
					(z >= 0) && (z < visLightListVoxelCountZ);

				if (coordIsValid)
				{
					VisibleLightList &visLightList = this->visLightLists.get(x, z);
					if (!visLightList.isFull())
					{
						visLightList.add(visLightID);
					}
				}
			}
		}
	}

	// Sort all of the touched voxel columns' light references by distance (shading optimization).
	const BufferView<const VisibleLight> visLightsView(this->visibleLights.data(),
		static_cast<int>(this->visibleLights.size()));

	for (WEInt z = 0; z < this->visLightLists.getHeight(); z++)
	{
		for (SNInt x = 0; x < this->visLightLists.getWidth(); x++)
		{
			VisibleLightList &visLightList = this->visLightLists.get(x, z);
			if (visLightList.count >= 2)
			{
				const NewInt2 voxel(x + minAbsoluteChunkVoxel.x, z + minAbsoluteChunkVoxel.y);

				// Default to the middle of the main floor for now (voxel columns aren't really in 3D).
				const Double3 voxelColumnPoint(
					static_cast<SNDouble>(voxel.x) + 0.50,
					ceilingHeight * 1.50,
					static_cast<WEDouble>(voxel.y) + 0.50);

				visLightList.sortByNearest(voxelColumnPoint, visLightsView);
			}
		}
	}
}

VoxelFacing2D SoftwareRenderer::getInitialChasmFarFacing(SNInt voxelX, WEInt voxelZ,
	const NewDouble2 &eye, const Ray &ray)
{
	// Angle of the ray from the camera eye.
	const Radians angle = MathUtils::fullAtan2(-ray.dirX, -ray.dirZ);

	// Corners in world space.
	NewDouble2 topLeftCorner, topRightCorner, bottomLeftCorner, bottomRightCorner;
	RendererUtils::getVoxelCorners2D(voxelX, voxelZ, &topLeftCorner, &topRightCorner,
		&bottomLeftCorner, &bottomRightCorner);

	const NewDouble2 upLeft = (topLeftCorner - eye).normalized();
	const NewDouble2 upRight = (topRightCorner - eye).normalized();
	const NewDouble2 downLeft = (bottomLeftCorner - eye).normalized();
	const NewDouble2 downRight = (bottomRightCorner - eye).normalized();
	const Radians upLeftAngle = MathUtils::fullAtan2(upLeft);
	const Radians upRightAngle = MathUtils::fullAtan2(upRight);
	const Radians downLeftAngle = MathUtils::fullAtan2(downLeft);
	const Radians downRightAngle = MathUtils::fullAtan2(downRight);

	// Find which range the ray's angle lies within.
	if ((angle < upRightAngle) || (angle > downRightAngle))
	{
		return VoxelFacing2D::NegativeZ;
	}
	else if (angle < upLeftAngle)
	{
		return VoxelFacing2D::NegativeX;
	}
	else if (angle < downLeftAngle)
	{
		return VoxelFacing2D::PositiveZ;
	}
	else
	{
		return VoxelFacing2D::PositiveX;
	}
}

VoxelFacing2D SoftwareRenderer::getChasmFarFacing(SNInt voxelX, WEInt voxelZ, 
	VoxelFacing2D nearFacing, const Camera &camera, const Ray &ray)
{
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
	const NewDouble2 eye2D(absoluteEye.x, absoluteEye.z);
	
	// Angle of the ray from the camera eye.
	const Radians angle = MathUtils::fullAtan2(-ray.dirX, -ray.dirZ);
	
	// Corners in world space.
	NewDouble2 topLeftCorner, topRightCorner, bottomLeftCorner, bottomRightCorner;
	RendererUtils::getVoxelCorners2D(voxelX, voxelZ, &topLeftCorner, &topRightCorner,
		&bottomLeftCorner, &bottomRightCorner);

	const NewDouble2 upLeft = (topLeftCorner - eye2D).normalized();
	const NewDouble2 upRight = (topRightCorner - eye2D).normalized();
	const NewDouble2 downLeft = (bottomLeftCorner - eye2D).normalized();
	const NewDouble2 downRight = (bottomRightCorner - eye2D).normalized();
	const Radians upLeftAngle = MathUtils::fullAtan2(upLeft);
	const Radians upRightAngle = MathUtils::fullAtan2(upRight);
	const Radians downLeftAngle = MathUtils::fullAtan2(downLeft);
	const Radians downRightAngle = MathUtils::fullAtan2(downRight);

	// Find which side it starts on then do some checks against line angles. When the
	// ray origin's voxel is at a diagonal to the voxel, ignore the corner and two
	// sides closest to that origin.
	if (nearFacing == VoxelFacing2D::PositiveX)
	{
		// Starts somewhere on (1.0, z).
		if (absoluteEyeVoxel.z > voxelZ)
		{
			// Ignore bottom-left corner.
			if (angle < upRightAngle)
			{
				return VoxelFacing2D::NegativeZ;
			}
			else
			{
				return VoxelFacing2D::NegativeX;
			}
		}
		else if (absoluteEyeVoxel.z < voxelZ)
		{
			// Ignore bottom-right corner.
			if (angle < upLeftAngle)
			{
				return VoxelFacing2D::NegativeX;
			}
			else
			{
				return VoxelFacing2D::PositiveZ;
			}
		}
		else
		{
			if ((angle > upLeftAngle) && (angle < downLeftAngle))
			{
				return VoxelFacing2D::PositiveZ;
			}
			else if ((angle > upRightAngle) && (angle < upLeftAngle))
			{
				return VoxelFacing2D::NegativeX;
			}
			else
			{
				return VoxelFacing2D::NegativeZ;
			}
		}
	}
	else if (nearFacing == VoxelFacing2D::NegativeX)
	{
		// Starts somewhere on (0.0, z).
		if (absoluteEyeVoxel.z > voxelZ)
		{
			// Ignore top-left corner.
			if ((angle < downRightAngle) && (angle > downLeftAngle))
			{
				return VoxelFacing2D::PositiveX;
			}
			else
			{
				return VoxelFacing2D::NegativeZ;
			}
		}
		else if (absoluteEyeVoxel.z < voxelZ)
		{
			// Ignore top-right corner.
			if (angle < downLeftAngle)
			{
				return VoxelFacing2D::PositiveZ;
			}
			else
			{
				return VoxelFacing2D::PositiveX;
			}
		}
		else
		{
			if ((angle < downLeftAngle) && (angle > upLeftAngle))
			{
				return VoxelFacing2D::PositiveZ;
			}
			else if ((angle < downRightAngle) && (angle > downLeftAngle))
			{
				return VoxelFacing2D::PositiveX;
			}
			else
			{
				return VoxelFacing2D::NegativeZ;
			}
		}
	}				
	else if (nearFacing == VoxelFacing2D::PositiveZ)
	{
		// Starts somewhere on (x, 1.0).
		if (absoluteEyeVoxel.x > voxelX)
		{
			// Ignore bottom-left corner.
			if ((angle > upRightAngle) && (angle < upLeftAngle))
			{
				return VoxelFacing2D::NegativeX;
			}
			else
			{
				return VoxelFacing2D::NegativeZ;
			}
		}
		else if (absoluteEyeVoxel.x < voxelX)
		{
			// Ignore top-left corner.
			if ((angle < downRightAngle) && (angle > downLeftAngle))
			{
				return VoxelFacing2D::PositiveX;
			}
			else
			{
				return VoxelFacing2D::NegativeZ;
			}
		}
		else
		{
			if ((angle < downRightAngle) && (angle > downLeftAngle))
			{
				return VoxelFacing2D::PositiveX;
			}
			else if ((angle < upLeftAngle) && (angle > upRightAngle))
			{
				return VoxelFacing2D::NegativeX;
			}
			else
			{
				return VoxelFacing2D::NegativeZ;
			}
		}
	}
	else
	{
		// Starts somewhere on (x, 0.0).
		if (absoluteEyeVoxel.x > voxelX)
		{
			// Ignore bottom-right corner.
			if (angle < upLeftAngle)
			{
				return VoxelFacing2D::NegativeX;
			}
			else
			{
				return VoxelFacing2D::PositiveZ;
			}
		}
		else if (absoluteEyeVoxel.x < voxelX)
		{
			// Ignore top-right corner.
			if (angle > downLeftAngle)
			{
				return VoxelFacing2D::PositiveX;
			}
			else
			{
				return VoxelFacing2D::PositiveZ;
			}
		}
		else
		{
			if (angle < upLeftAngle)
			{
				return VoxelFacing2D::NegativeX;
			}
			else if (angle < downLeftAngle)
			{
				return VoxelFacing2D::PositiveZ;
			}
			else
			{
				return VoxelFacing2D::PositiveX;
			}
		}
	}
}

void SoftwareRenderer::getChasmTextureGroupTexture(const ChasmTextureGroups &textureGroups,
	ArenaTypes::ChasmType chasmType, double chasmAnimPercent, const ChasmTexture **outTexture)
{
	const int chasmID = RendererUtils::getChasmIdFromType(chasmType);
	const auto groupIter = textureGroups.find(chasmID);
	if (groupIter == textureGroups.end())
	{
		DebugCrash("Missing chasm texture group " + std::to_string(chasmID) + ".");
		*outTexture = nullptr;
		return;
	}

	const auto &textureGroup = groupIter->second;
	const int groupSize = static_cast<int>(textureGroup.size());
	if (groupSize == 0)
	{
		DebugCrash("Empty chasm texture group " + std::to_string(chasmID) + ".");
		*outTexture = nullptr;
		return;
	}

	const double groupRealIndex = MathUtils::getRealIndex(groupSize, chasmAnimPercent);
	const int animIndex = static_cast<int>(groupRealIndex);
	*outTexture = &textureGroup[animIndex];
}

const SoftwareRenderer::VisibleLight &SoftwareRenderer::getVisibleLightByID(
	const BufferView<const VisibleLight> &visLights, VisibleLightList::LightID lightID)
{
	return visLights.get(lightID);
}

const SoftwareRenderer::VisibleLightList &SoftwareRenderer::getVisibleLightList(
	const BufferView2D<const VisibleLightList> &visLightLists, SNInt voxelX, WEInt voxelZ,
	SNInt cameraVoxelX, WEInt cameraVoxelZ, SNInt gridWidth, WEInt gridDepth, int chunkDistance)
{
	// Convert new voxel grid coordinates to potentially-visible light list space
	// (chunk space but its origin depends on the camera).
	const NewInt2 newVoxel(voxelX, voxelZ);

	// Visible light lists are relative to the potentially visible chunks.
	const CoordInt2 cameraChunkCoord = VoxelUtils::newVoxelToCoord(NewInt2(cameraVoxelX, cameraVoxelZ));

	ChunkInt2 minChunk, maxChunk;
	ChunkUtils::getSurroundingChunks(cameraChunkCoord.chunk, chunkDistance, &minChunk, &maxChunk);

	// Get the closest-to-origin voxel in the potentially visible chunks so we can do some
	// relative chunk calculations.
	const NewInt2 minAbsoluteChunkVoxel = VoxelUtils::chunkVoxelToNewVoxel(minChunk, VoxelInt2(0, 0));

	const int visLightListX = newVoxel.x - minAbsoluteChunkVoxel.x;
	const int visLightListY = newVoxel.y - minAbsoluteChunkVoxel.y;

	// @todo: temp hack to avoid crash from bad coordinate math. Not sure how to fix it
	// because sometimes the XY is too low or too high, so it doesn't feel like a simple
	// off-by- +/- one in some coordinate system transform :/ it'll hopefully get fixed
	// when NewInt2 gets removed.
	const bool coordIsValid =
		(visLightListX >= 0) && (visLightListX < visLightLists.getWidth()) &&
		(visLightListY >= 0) && (visLightListY < visLightLists.getHeight());

	if (!coordIsValid)
	{
		return visLightLists.get(
			std::clamp(visLightListX, 0, visLightLists.getWidth() - 1),
			std::clamp(visLightListY, 0, visLightLists.getHeight() - 1));
	}

	return visLightLists.get(visLightListX, visLightListY);
}

SoftwareRenderer::DrawRange SoftwareRenderer::makeDrawRange(const Double3 &startPoint,
	const Double3 &endPoint, const Camera &camera, const FrameView &frame)
{
	const double yProjStart = RendererUtils::getProjectedY(
		startPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double yProjEnd = RendererUtils::getProjectedY(
		endPoint, camera.transform, camera.yShear) * frame.heightReal;
	const int yStart = RendererUtils::getLowerBoundedPixel(yProjStart, frame.height);
	const int yEnd = RendererUtils::getUpperBoundedPixel(yProjEnd, frame.height);

	return DrawRange(yProjStart, yProjEnd, yStart, yEnd);
}

std::array<SoftwareRenderer::DrawRange, 2> SoftwareRenderer::makeDrawRangeTwoPart(
	const Double3 &startPoint, const Double3 &midPoint, const Double3 &endPoint,
	const Camera &camera, const FrameView &frame)
{
	const double startYProjStart = RendererUtils::getProjectedY(
		startPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double startYProjEnd = RendererUtils::getProjectedY(
		midPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double endYProjEnd = RendererUtils::getProjectedY(
		endPoint, camera.transform, camera.yShear) * frame.heightReal;

	const int startYStart = RendererUtils::getLowerBoundedPixel(startYProjStart, frame.height);
	const int startYEnd = RendererUtils::getUpperBoundedPixel(startYProjEnd, frame.height);
	const int endYStart = startYEnd;
	const int endYEnd = RendererUtils::getUpperBoundedPixel(endYProjEnd, frame.height);

	return std::array<DrawRange, 2>
	{
		DrawRange(startYProjStart, startYProjEnd, startYStart, startYEnd),
		DrawRange(startYProjEnd, endYProjEnd, endYStart, endYEnd)
	};
}

std::array<SoftwareRenderer::DrawRange, 3> SoftwareRenderer::makeDrawRangeThreePart(
	const Double3 &startPoint, const Double3 &midPoint1, const Double3 &midPoint2,
	const Double3 &endPoint, const Camera &camera, const FrameView &frame)
{
	const double startYProjStart = RendererUtils::getProjectedY(
		startPoint, camera.transform, camera.yShear) * frame.heightReal;
	const double startYProjEnd = RendererUtils::getProjectedY(
		midPoint1, camera.transform, camera.yShear) * frame.heightReal;
	const double mid1YProjEnd = RendererUtils::getProjectedY(
		midPoint2, camera.transform, camera.yShear) * frame.heightReal;
	const double mid2YProjEnd = RendererUtils::getProjectedY(
		endPoint, camera.transform, camera.yShear) * frame.heightReal;

	const int startYStart = RendererUtils::getLowerBoundedPixel(startYProjStart, frame.height);
	const int startYEnd = RendererUtils::getUpperBoundedPixel(startYProjEnd, frame.height);
	const int mid1YStart = startYEnd;
	const int mid1YEnd = RendererUtils::getUpperBoundedPixel(mid1YProjEnd, frame.height);
	const int mid2YStart = mid1YEnd;
	const int mid2YEnd = RendererUtils::getUpperBoundedPixel(mid2YProjEnd, frame.height);

	return std::array<DrawRange, 3>
	{
		DrawRange(startYProjStart, startYProjEnd, startYStart, startYEnd),
		DrawRange(startYProjEnd, mid1YProjEnd, mid1YStart, mid1YEnd),
		DrawRange(mid1YProjEnd, mid2YProjEnd, mid2YStart, mid2YEnd)
	};
}

void SoftwareRenderer::getSkyGradientProjectedYRange(const Camera &camera, double &projectedYTop,
	double &projectedYBottom)
{
	const NewDouble3 absoluteEyePoint = VoxelUtils::coordToNewPoint(camera.eye);

	// Get two points some arbitrary distance away from the camera to use as the top
	// and bottom reference points of the sky gradient.
	const Double3 forward = Double3(camera.forwardX, 0.0, camera.forwardZ).normalized();

	// Determine the sky gradient's position on-screen by getting the projected Y percentages for
	// the start and end. If these values are less than 0 or greater than 1, they are off-screen.
	projectedYTop = [&camera, &absoluteEyePoint, &forward]()
	{
		const Double3 gradientTopPoint = [&absoluteEyePoint, &forward]()
		{
			// Top of the sky gradient is some angle above the horizon.
			const double gradientAngleRadians =
				SoftwareRenderer::SKY_GRADIENT_ANGLE * Constants::DegToRad;

			// Height of the gradient's triangle with width of 1 and angle of 30 degrees.
			const double upPercent = std::tan(gradientAngleRadians);
			const Double3 up = Double3::UnitY;

			// Direction from camera eye to the top of the sky gradient.
			const Double3 gradientTopDir = (forward + (up * upPercent)).normalized();

			return absoluteEyePoint + gradientTopDir;
		}();

		return RendererUtils::getProjectedY(gradientTopPoint, camera.transform, camera.yShear);
	}();

	projectedYBottom = [&camera, &absoluteEyePoint, &forward]()
	{
		const Double3 gradientBottomPoint = absoluteEyePoint + forward;
		return RendererUtils::getProjectedY(gradientBottomPoint, camera.transform, camera.yShear);
	}();
}

double SoftwareRenderer::getSkyGradientPercent(double projectedY, double projectedYTop,
	double projectedYBottom)
{
	// The sky gradient percent is 0 at the horizon and just below 1 at the top (for sky texture
	// coordinates).
	return Constants::JustBelowOne - std::clamp(
		(projectedY - projectedYTop) / (projectedYBottom - projectedYTop),
		0.0, Constants::JustBelowOne);
}

Double3 SoftwareRenderer::getSkyGradientRowColor(double gradientPercent, const ShadingInfo &shadingInfo)
{
	// Determine which sky color index the percent falls into, and how much of that
	// color to interpolate with the next one.
	const auto &skyColors = shadingInfo.skyColors;
	const int skyColorCount = static_cast<int>(skyColors.size());
	const double realIndex = MathUtils::getRealIndex(skyColorCount, gradientPercent);
	const double percent = realIndex - std::floor(realIndex);
	const int index = static_cast<int>(realIndex);
	const int nextIndex = std::clamp(index + 1, 0, skyColorCount - 1);
	const Double3 &color = skyColors.at(index);
	const Double3 &nextColor = skyColors.at(nextIndex);
	return color.lerp(nextColor, percent);
}

bool SoftwareRenderer::findDiag1Intersection(SNInt voxelX, WEInt voxelZ,
	const NewDouble2 &nearPoint, const NewDouble2 &farPoint, RayHit &hit)
{
	// Start, middle, and end points of the diagonal line segment relative to the grid.
	NewDouble2 diagStart, diagMiddle, diagEnd;
	RendererUtils::getDiag1Points2D(voxelX, voxelZ, &diagStart, &diagMiddle, &diagEnd);

	// Normals for the left and right faces of the wall, facing down-right and up-left
	// respectively (magic number is sqrt(2) / 2).
	const Double3 leftNormal(0.7071068, 0.0, -0.7071068);
	const Double3 rightNormal(-0.7071068, 0.0, 0.7071068);
	
	// An intersection occurs if the near point and far point are on different sides 
	// of the diagonal line, or if the near point lies on the diagonal line. No need
	// to normalize the (localPoint - diagMiddle) vector because it's just checking
	// if it's greater than zero.
	const NewDouble2 leftNormal2D(leftNormal.x, leftNormal.z);
	const bool nearOnLeft = leftNormal2D.dot(nearPoint - diagMiddle) >= 0.0;
	const bool farOnLeft = leftNormal2D.dot(farPoint - diagMiddle) >= 0.0;
	const bool intersectionOccurred = (nearOnLeft && !farOnLeft) || (!nearOnLeft && farOnLeft);

	// Only set the output data if an intersection occurred.
	if (intersectionOccurred)
	{
		// Change in X and change in Z of the incoming ray across the voxel.
		const SNDouble dx = farPoint.x - nearPoint.x;
		const WEDouble dz = farPoint.y - nearPoint.y;

		// The hit coordinate is a 0->1 value representing where the diagonal was hit.
		const double hitCoordinate = [&nearPoint, &diagStart, dx, dz]()
		{
			// Special cases: when the slope is horizontal or vertical. This method treats
			// the X axis as the vertical axis and the Z axis as the horizontal axis.
			const bool isHorizontal = std::abs(dx) < Constants::Epsilon;
			const bool isVertical = std::abs(dz) < Constants::Epsilon;

			if (isHorizontal)
			{
				// The X axis intercept is the intersection coordinate.
				return nearPoint.x - diagStart.x;
			}
			else if (isVertical)
			{
				// The Z axis intercept is the intersection coordinate.
				return nearPoint.y - diagStart.y;
			}
			else
			{
				// Slope of the diagonal line (trivial, x = z).
				constexpr double diagSlope = 1.0;

				// Vertical axis intercept of the diagonal line.
				const double diagXIntercept = diagStart.x - diagStart.y;

				// Slope of the incoming ray.
				const double raySlope = dx / dz;

				// Get the vertical axis intercept of the incoming ray.
				const double rayXIntercept = nearPoint.x - (raySlope * nearPoint.y);

				// General line intersection calculation.
				return ((rayXIntercept - diagXIntercept) / (diagSlope - raySlope)) - diagStart.y;
			}
		}();

		// Set the hit data.
		hit.u = std::clamp(hitCoordinate, 0.0, Constants::JustBelowOne);
		hit.point = diagStart + ((diagEnd - diagStart) * hitCoordinate);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.normal = nearOnLeft ? leftNormal : rightNormal;

		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool SoftwareRenderer::findDiag2Intersection(SNInt voxelX, WEInt voxelZ,
	const NewDouble2 &nearPoint, const NewDouble2 &farPoint, RayHit &hit)
{
	// Mostly a copy of findDiag1Intersection(), though with a couple different values
	// for the diagonal (end points, slope, etc.).

	// Start, middle, and end points of the diagonal line segment relative to the grid.
	NewDouble2 diagStart, diagMiddle, diagEnd;
	RendererUtils::getDiag2Points2D(voxelX, voxelZ, &diagStart, &diagMiddle, &diagEnd);

	// Normals for the left and right faces of the wall, facing down-left and up-right
	// respectively (magic number is sqrt(2) / 2).
	const Double3 leftNormal(0.7071068, 0.0, 0.7071068);
	const Double3 rightNormal(-0.7071068, 0.0, -0.7071068);

	// An intersection occurs if the near point and far point are on different sides 
	// of the diagonal line, or if the near point lies on the diagonal line. No need
	// to normalize the (localPoint - diagMiddle) vector because it's just checking
	// if it's greater than zero.
	const Double2 leftNormal2D(leftNormal.x, leftNormal.z);
	const bool nearOnLeft = leftNormal2D.dot(nearPoint - diagMiddle) >= 0.0;
	const bool farOnLeft = leftNormal2D.dot(farPoint - diagMiddle) >= 0.0;
	const bool intersectionOccurred = (nearOnLeft && !farOnLeft) || (!nearOnLeft && farOnLeft);

	// Only set the output data if an intersection occurred.
	if (intersectionOccurred)
	{
		// Change in X and change in Z of the incoming ray across the voxel.
		const SNDouble dx = farPoint.x - nearPoint.x;
		const WEDouble dz = farPoint.y - nearPoint.y;

		// The hit coordinate is a 0->1 value representing where the diagonal was hit.
		const double hitCoordinate = [&nearPoint, &diagStart, dx, dz]()
		{
			// Special cases: when the slope is horizontal or vertical. This method treats
			// the X axis as the vertical axis and the Z axis as the horizontal axis.
			const bool isHorizontal = std::abs(dx) < Constants::Epsilon;
			const bool isVertical = std::abs(dz) < Constants::Epsilon;

			if (isHorizontal)
			{
				// The X axis intercept is the compliment of the intersection coordinate.
				return Constants::JustBelowOne - (nearPoint.x - diagStart.x);
			}
			else if (isVertical)
			{
				// The Z axis intercept is the compliment of the intersection coordinate.
				return Constants::JustBelowOne - (nearPoint.y - diagStart.y);
			}
			else
			{
				// Slope of the diagonal line (trivial, x = -z).
				const double diagSlope = -1.0;

				// Vertical axis intercept of the diagonal line.
				const double diagXIntercept = diagStart.x + diagStart.y;

				// Slope of the incoming ray.
				const double raySlope = dx / dz;

				// Get the vertical axis intercept of the incoming ray.
				const double rayXIntercept = nearPoint.x - (raySlope * nearPoint.y);

				// General line intersection calculation.
				return ((rayXIntercept - diagXIntercept) / (diagSlope - raySlope)) - diagStart.y;
			}
		}();

		// Set the hit data.
		hit.u = std::clamp(Constants::JustBelowOne - hitCoordinate, 0.0, Constants::JustBelowOne);
		hit.point = diagStart + ((diagEnd - diagStart) * hitCoordinate);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.normal = nearOnLeft ? leftNormal : rightNormal;

		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool SoftwareRenderer::findInitialEdgeIntersection(SNInt voxelX, WEInt voxelZ, 
	VoxelFacing2D edgeFacing, bool flipped, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
	const Camera &camera, const Ray &ray, RayHit &hit)
{
	// Reuse the chasm facing code to find which face is intersected.
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewDouble2 absoluteEye2D(absoluteEye.x, absoluteEye.z);
	const VoxelFacing2D farFacing = SoftwareRenderer::getInitialChasmFarFacing(voxelX, voxelZ, absoluteEye2D, ray);

	// If the edge facing and far facing match, there's an intersection.
	if (edgeFacing == farFacing)
	{
		hit.innerZ = (farPoint - nearPoint).length();
		hit.u = [flipped, &farPoint, farFacing]()
		{
			const double uVal = [&farPoint, farFacing]()
			{
				if (farFacing == VoxelFacing2D::PositiveX)
				{
					return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
				}
				else if (farFacing == VoxelFacing2D::NegativeX)
				{
					return farPoint.y - std::floor(farPoint.y);
				}
				else if (farFacing == VoxelFacing2D::PositiveZ)
				{
					return farPoint.x - std::floor(farPoint.x);
				}
				else
				{
					return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
				}
			}();

			// Account for the possibility of the texture being flipped horizontally.
			return std::clamp(!flipped ? uVal : (Constants::JustBelowOne - uVal),
				0.0, Constants::JustBelowOne);
		}();

		hit.point = farPoint;
		hit.normal = -VoxelUtils::getNormal(farFacing);
		return true;
	}
	else
	{
		// No intersection.
		return false;
	}
}

bool SoftwareRenderer::findEdgeIntersection(SNInt voxelX, WEInt voxelZ, VoxelFacing2D edgeFacing,
	bool flipped, VoxelFacing2D nearFacing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
	double nearU, const Camera &camera, const Ray &ray, RayHit &hit)
{
	// If the edge facing and near facing match, the intersection is trivial.
	if (edgeFacing == nearFacing)
	{
		hit.innerZ = 0.0;
		hit.u = !flipped ? nearU : std::clamp(
			Constants::JustBelowOne - nearU, 0.0, Constants::JustBelowOne);
		hit.point = nearPoint;
		hit.normal = VoxelUtils::getNormal(nearFacing);
		return true;
	}
	else
	{
		// A search is needed to see whether an intersection occurred. Reuse the chasm
		// facing code to find what the far facing is.
		const VoxelFacing2D farFacing = SoftwareRenderer::getChasmFarFacing(
			voxelX, voxelZ, nearFacing, camera, ray);

		// If the edge facing and far facing match, there's an intersection.
		if (edgeFacing == farFacing)
		{
			hit.innerZ = (farPoint - nearPoint).length();
			hit.u = [flipped, &farPoint, farFacing]()
			{
				const double uVal = [&farPoint, farFacing]()
				{
					if (farFacing == VoxelFacing2D::PositiveX)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else if (farFacing == VoxelFacing2D::NegativeX)
					{
						return farPoint.y - std::floor(farPoint.y);
					}
					else if (farFacing == VoxelFacing2D::PositiveZ)
					{
						return farPoint.x - std::floor(farPoint.x);
					}
					else
					{
						return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
					}
				}();

				// Account for the possibility of the texture being flipped horizontally.
				return std::clamp(!flipped ? uVal : (Constants::JustBelowOne - uVal),
					0.0, Constants::JustBelowOne);
			}();

			hit.point = farPoint;
			hit.normal = -VoxelUtils::getNormal(farFacing);
			return true;
		}
		else
		{
			// No intersection.
			return false;
		}
	}
}

bool SoftwareRenderer::findInitialSwingingDoorIntersection(SNInt voxelX, WEInt voxelZ,
	double percentOpen, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, bool xAxis,
	const Camera &camera, const Ray &ray, RayHit &hit)
{
	// Decide which corner the door's hinge will be in, and create the line segment
	// that will be rotated based on percent open.
	NewDouble2 interpStart;
	const NewDouble2 pivot = [voxelX, voxelZ, xAxis, &interpStart]()
	{
		const NewInt2 corner = [voxelX, voxelZ, xAxis, &interpStart]()
		{
			if (xAxis)
			{
				interpStart = CardinalDirection::South;
				return NewInt2(voxelX, voxelZ);
			}
			else
			{
				interpStart = CardinalDirection::West;
				return NewInt2(voxelX + 1, voxelZ);
			}
		}();

		const NewDouble2 cornerReal(
			static_cast<SNDouble>(corner.x),
			static_cast<WEDouble>(corner.y));

		// Bias the pivot towards the voxel center slightly to avoid Z-fighting with
		// adjacent walls.
		const NewDouble2 voxelCenter(
			static_cast<SNDouble>(voxelX) + 0.50,
			static_cast<WEDouble>(voxelZ) + 0.50);
		const NewDouble2 bias = (voxelCenter - cornerReal) * Constants::Epsilon;
		return cornerReal + bias;
	}();

	// Use the left perpendicular vector of the door's closed position as the 
	// fully open position.
	const NewDouble2 interpEnd = interpStart.leftPerp();

	// Actual position of the door in its rotation, represented as a vector.
	const NewDouble2 doorVec = interpStart.lerp(interpEnd, 1.0 - percentOpen).normalized();

	// Use back-face culling with swinging doors so it's not obstructing the player's
	// view as much when it's opening.
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewDouble2 eye2D(absoluteEye.x, absoluteEye.z);
	const bool isFrontFace = (eye2D - pivot).normalized().dot(doorVec.leftPerp()) > 0.0;

	if (isFrontFace)
	{
		// Vector cross product in 2D, returns a scalar.
		auto cross = [](const NewDouble2 &a, const NewDouble2 &b)
		{
			return (a.x * b.y) - (b.x * a.y);
		};

		// Solve line segment intersection between the incoming ray and the door.
		const NewDouble2 p1 = pivot;
		const NewDouble2 v1 = doorVec;
		const NewDouble2 p2 = nearPoint;
		const NewDouble2 v2 = farPoint - nearPoint;

		// Percent from p1 to (p1 + v1).
		const double t = cross(p2 - p1, v2) / cross(v1, v2);

		// See if the two line segments intersect.
		if ((t >= 0.0) && (t < 1.0))
		{
			// Hit.
			hit.point = p1 + (v1 * t);
			hit.innerZ = (hit.point - nearPoint).length();
			hit.u = t;
			hit.normal = [&v1]()
			{
				const NewDouble2 norm2D = v1.rightPerp();
				return Double3(norm2D.x, 0.0, norm2D.y);
			}();

			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else
	{
		// Cull back face.
		return false;
	}
}

bool SoftwareRenderer::findInitialDoorIntersection(SNInt voxelX, WEInt voxelZ,
	ArenaTypes::DoorType doorType, double percentOpen, const NewDouble2 &nearPoint,
	const NewDouble2 &farPoint, const Camera &camera, const Ray &ray,
	const VoxelGrid &voxelGrid, RayHit &hit)
{
	// Determine which axis the door should open/close for (either X or Z).
	const bool xAxis = [voxelX, voxelZ, &voxelGrid]()
	{
		// Check adjacent voxels on the X axis for air.
		auto voxelIsAir = [&voxelGrid](SNInt x, WEInt z)
		{
			const bool insideGrid = (x >= 0) && (x < voxelGrid.getWidth()) &&
				(z >= 0) && (z < voxelGrid.getDepth());

			if (insideGrid)
			{
				const uint16_t voxelID = voxelGrid.getVoxel(x, 1, z);
				const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
				return voxelDef.type == ArenaTypes::VoxelType::None;
			}
			else
			{
				// Anything past the map edge is considered air.
				return true;
			}
		};

		// If voxels (x - 1, z) and (x + 1, z) are empty, return true.
		return voxelIsAir(voxelX - 1, voxelZ) && voxelIsAir(voxelX + 1, voxelZ);
	}();

	// If the current intersection surface is along one of the voxel's edges, treat the door
	// like a wall by basing intersection calculations on the far facing.
	const bool useFarFacing = [doorType, percentOpen]()
	{
		const bool isClosed = percentOpen == 0.0;
		return isClosed ||
			(doorType == ArenaTypes::DoorType::Sliding) ||
			(doorType == ArenaTypes::DoorType::Raising) ||
			(doorType == ArenaTypes::DoorType::Splitting);
	}();

	if (useFarFacing)
	{
		// Treat the door like a wall. Reuse the chasm facing code to find which face is intersected.
		const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
		const NewDouble2 absoluteEye2D(absoluteEye.x, absoluteEye.z);
		const VoxelFacing2D farFacing = SoftwareRenderer::getInitialChasmFarFacing(voxelX, voxelZ, absoluteEye2D, ray);
		const VoxelFacing2D doorFacing = xAxis ? VoxelFacing2D::PositiveX : VoxelFacing2D::PositiveZ;

		if (doorFacing == farFacing)
		{
			// The ray intersected the target facing. See if the door itself was intersected
			// and write out hit data based on the door type.
			const double farU = [&farPoint, xAxis]()
			{
				const double uVal = [&farPoint, xAxis]()
				{
					if (xAxis)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else
					{
						return farPoint.x - std::floor(farPoint.x);
					}
				}();

				return std::clamp(uVal, 0.0, Constants::JustBelowOne);
			}();

			if (doorType == ArenaTypes::DoorType::Swinging)
			{
				// Treat like a wall.
				hit.innerZ = (farPoint - nearPoint).length();
				hit.u = farU;
				hit.point = farPoint;
				hit.normal = -VoxelUtils::getNormal(farFacing);
				return true;
			}
			else if (doorType == ArenaTypes::DoorType::Sliding)
			{
				// If far U coordinate is within percent closed, it's a hit. At 100% open,
				// a sliding door is still partially visible.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double visibleAmount = 1.0 - ((1.0 - minVisible) * percentOpen);
				if (visibleAmount > farU)
				{
					hit.innerZ = (farPoint - nearPoint).length();
					hit.u = std::clamp(farU + (1.0 - visibleAmount), 0.0, Constants::JustBelowOne);
					hit.point = farPoint;
					hit.normal = -VoxelUtils::getNormal(farFacing);
					return true;
				}
				else
				{
					// No hit.
					return false;
				}
			}
			else if (doorType == ArenaTypes::DoorType::Raising)
			{
				// Raising doors are always hit.
				hit.innerZ = (farPoint - nearPoint).length();
				hit.u = farU;
				hit.point = farPoint;
				hit.normal = -VoxelUtils::getNormal(farFacing);
				return true;
			}
			else if (doorType == ArenaTypes::DoorType::Splitting)
			{
				// If far U coordinate is within percent closed on left or right half, it's a hit.
				// At 100% open, a splitting door is still partially visible.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const bool leftHalf = farU < 0.50;
				const bool rightHalf = farU > 0.50;
				double leftVisAmount, rightVisAmount;
				const bool success = [percentOpen, farU, minVisible, leftHalf, rightHalf,
					&leftVisAmount, &rightVisAmount]()
				{
					if (leftHalf)
					{
						// Left half.
						leftVisAmount = 0.50 - ((0.50 - minVisible) * percentOpen);
						return farU <= leftVisAmount;
					}
					else if (rightHalf)
					{
						// Right half.
						rightVisAmount = 0.50 + ((0.50 - minVisible) * percentOpen);
						return farU >= rightVisAmount;
					}
					else
					{
						// Midpoint (only when door is completely closed).
						return percentOpen == 0.0;
					}
				}();

				if (success)
				{
					// Hit.
					hit.innerZ = (farPoint - nearPoint).length();
					hit.u = [farU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
					{
						const double u = [farU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
						{
							if (leftHalf)
							{
								return (farU + 0.50) - leftVisAmount;
							}
							else if (rightHalf)
							{
								return (farU + 0.50) - rightVisAmount;
							}
							else
							{
								// Midpoint.
								return 0.50;
							}
						}();

						return std::clamp(u, 0.0, Constants::JustBelowOne);
					}();

					hit.point = farPoint;
					hit.normal = -VoxelUtils::getNormal(farFacing);

					return true;
				}
				else
				{
					// No hit.
					return false;
				}
			}
			else
			{
				// Invalid door type.
				return false;
			}
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (doorType == ArenaTypes::DoorType::Swinging)
	{
		return SoftwareRenderer::findInitialSwingingDoorIntersection(voxelX, voxelZ, percentOpen,
			nearPoint, farPoint, xAxis, camera, ray, hit);
	}
	else
	{
		// Invalid door type.
		return false;
	}
}

bool SoftwareRenderer::findSwingingDoorIntersection(SNInt voxelX, WEInt voxelZ,
	double percentOpen, VoxelFacing2D nearFacing, const NewDouble2 &nearPoint,
	const NewDouble2 &farPoint, double nearU, RayHit &hit)
{
	// Decide which corner the door's hinge will be in, and create the line segment
	// that will be rotated based on percent open.
	NewDouble2 interpStart;
	const NewDouble2 pivot = [voxelX, voxelZ, nearFacing, &interpStart]()
	{
		const NewInt2 corner = [voxelX, voxelZ, nearFacing, &interpStart]()
		{
			if (nearFacing == VoxelFacing2D::PositiveX)
			{
				interpStart = CardinalDirection::North;
				return NewInt2(voxelX + 1, voxelZ + 1);
			}
			else if (nearFacing == VoxelFacing2D::NegativeX)
			{
				interpStart = CardinalDirection::South;
				return NewInt2(voxelX, voxelZ);
			}
			else if (nearFacing == VoxelFacing2D::PositiveZ)
			{
				interpStart = CardinalDirection::East;
				return NewInt2(voxelX, voxelZ + 1);
			}
			else if (nearFacing == VoxelFacing2D::NegativeZ)
			{
				interpStart = CardinalDirection::West;
				return NewInt2(voxelX + 1, voxelZ);
			}
			else
			{
				DebugUnhandledReturnMsg(NewInt2, std::to_string(static_cast<int>(nearFacing)));
			}
		}();

		const NewDouble2 cornerReal(
			static_cast<SNDouble>(corner.x),
			static_cast<WEDouble>(corner.y));

		// Bias the pivot towards the voxel center slightly to avoid Z-fighting with
		// adjacent walls.
		const NewDouble2 voxelCenter(
			static_cast<SNDouble>(voxelX) + 0.50,
			static_cast<WEDouble>(voxelZ) + 0.50);
		const NewDouble2 bias = (voxelCenter - cornerReal) * Constants::Epsilon;
		return cornerReal + bias;
	}();

	// Use the left perpendicular vector of the door's closed position as the 
	// fully open position.
	const NewDouble2 interpEnd = interpStart.leftPerp();

	// Actual position of the door in its rotation, represented as a vector.
	const NewDouble2 doorVec = interpStart.lerp(interpEnd, 1.0 - percentOpen).normalized();

	// Vector cross product in 2D, returns a scalar.
	auto cross = [](const NewDouble2 &a, const NewDouble2 &b)
	{
		return (a.x * b.y) - (b.x * a.y);
	};

	// Solve line segment intersection between the incoming ray and the door.
	const NewDouble2 p1 = pivot;
	const NewDouble2 v1 = doorVec;
	const NewDouble2 p2 = nearPoint;
	const NewDouble2 v2 = farPoint - nearPoint;

	// Percent from p1 to (p1 + v1).
	const double t = cross(p2 - p1, v2) / cross(v1, v2);

	// See if the two line segments intersect.
	if ((t >= 0.0) && (t < 1.0))
	{
		// Hit.
		hit.point = p1 + (v1 * t);
		hit.innerZ = (hit.point - nearPoint).length();
		hit.u = t;
		hit.normal = [&v1]()
		{
			const NewDouble2 norm2D = v1.rightPerp();
			return Double3(norm2D.x, 0.0, norm2D.y);
		}();

		return true;
	}
	else
	{
		// No hit.
		return false;
	}
}

bool SoftwareRenderer::findDoorIntersection(SNInt voxelX, WEInt voxelZ, 
	ArenaTypes::DoorType doorType, double percentOpen, VoxelFacing2D nearFacing,
	const NewDouble2 &nearPoint, const NewDouble2 &farPoint, double nearU, RayHit &hit)
{
	// Check trivial case first: whether the door is closed.
	const bool isClosed = percentOpen == 0.0;

	if (isClosed)
	{
		// Treat like a wall.
		hit.innerZ = 0.0;
		hit.u = nearU;
		hit.point = nearPoint;
		hit.normal = VoxelUtils::getNormal(nearFacing);
		return true;
	}
	else if (doorType == ArenaTypes::DoorType::Swinging)
	{
		return SoftwareRenderer::findSwingingDoorIntersection(voxelX, voxelZ, percentOpen,
			nearFacing, nearPoint, farPoint, nearU, hit);
	}
	else if (doorType == ArenaTypes::DoorType::Sliding)
	{
		// If near U coordinate is within percent closed, it's a hit. At 100% open,
		// a sliding door is still partially visible.
		const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
		const double visibleAmount = 1.0 - ((1.0 - minVisible) * percentOpen);
		if (visibleAmount > nearU)
		{
			hit.innerZ = 0.0;
			hit.u = std::clamp(nearU + (1.0 - visibleAmount), 0.0, Constants::JustBelowOne);
			hit.point = nearPoint;
			hit.normal = VoxelUtils::getNormal(nearFacing);
			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else if (doorType == ArenaTypes::DoorType::Raising)
	{
		// Raising doors are always hit.
		hit.innerZ = 0.0;
		hit.u = nearU;
		hit.point = nearPoint;
		hit.normal = VoxelUtils::getNormal(nearFacing);
		return true;
	}
	else if (doorType == ArenaTypes::DoorType::Splitting)
	{
		// If near U coordinate is within percent closed on left or right half, it's a hit.
		// At 100% open, a splitting door is still partially visible.
		const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
		const bool leftHalf = nearU < 0.50;
		const bool rightHalf = nearU > 0.50;
		double leftVisAmount, rightVisAmount;
		const bool success = [percentOpen, nearU, minVisible, leftHalf, rightHalf,
			&leftVisAmount, &rightVisAmount]()
		{
			if (leftHalf)
			{
				// Left half.
				leftVisAmount = 0.50 - ((0.50 - minVisible) * percentOpen);
				return nearU <= leftVisAmount;
			}
			else if (rightHalf)
			{
				// Right half.
				rightVisAmount = 0.50 + ((0.50 - minVisible) * percentOpen);
				return nearU >= rightVisAmount;
			}
			else
			{
				// Midpoint (only when door is completely closed).
				return percentOpen == 0.0;
			}
		}();
		
		if (success)
		{
			// Hit.
			hit.innerZ = 0.0;
			hit.u = [nearU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
			{
				const double u = [nearU, leftHalf, rightHalf, leftVisAmount, rightVisAmount]()
				{
					if (leftHalf)
					{
						return (nearU + 0.50) - leftVisAmount;
					}
					else if (rightHalf)
					{
						return (nearU + 0.50) - rightVisAmount;
					}
					else
					{
						// Midpoint.
						return 0.50;
					}
				}();
				
				return std::clamp(u, 0.0, Constants::JustBelowOne);
			}();

			hit.point = nearPoint;
			hit.normal = VoxelUtils::getNormal(nearFacing);

			return true;
		}
		else
		{
			// No hit.
			return false;
		}
	}
	else
	{
		// Invalid door type.
		return false;
	}
}

void SoftwareRenderer::getLightVisibilityData(const NewDouble3 &flatPosition, double flatHeight,
	int lightIntensity, const NewDouble2 &eye2D, const NewDouble2 &cameraDir, Degrees fovX,
	double viewDistance, LightVisibilityData *outVisData)
{
	// Put the light position at the center of the entity.
	// @todo: maybe base it on the first frame so there's no jitter if the entity height is variable?
	const double entityHalfHeight = flatHeight * 0.50;
	const Double3 lightPosition = flatPosition + (Double3::UnitY * entityHalfHeight);
	const NewDouble2 lightPosition2D(lightPosition.x, lightPosition.z);

	// Point at max view distance away from current camera view.
	const NewDouble2 cameraMaxPoint = eye2D + (cameraDir * viewDistance);

	// Distance from max view point to left or right far frustum corner.
	const double frustumHalfWidth = viewDistance * std::tan((fovX * 0.50) * Constants::DegToRad);

	const NewDouble2 cameraFrustumP0 = eye2D;
	const NewDouble2 cameraFrustumP1 = cameraMaxPoint + (cameraDir.rightPerp() * frustumHalfWidth);
	const NewDouble2 cameraFrustumP2 = cameraMaxPoint + (cameraDir.leftPerp() * frustumHalfWidth);

	const double lightRadius = static_cast<double>(lightIntensity);
	const bool intersectsFrustum = MathUtils::triangleCircleIntersection(
		cameraFrustumP0, cameraFrustumP1, cameraFrustumP2, lightPosition2D, lightRadius);

	outVisData->init(lightPosition, lightRadius, intersectsFrustum);
}

template <bool CappedSum>
double SoftwareRenderer::getLightContributionAtPoint(const NewDouble2 &point,
	const BufferView<const VisibleLight> &visLights, const VisibleLightList &visLightList)
{
	double lightContributionPercent = 0.0;
	for (int i = 0; i < visLightList.count; i++)
	{
		const VisibleLightList::LightID lightID = visLightList.lightIDs[i];
		const VisibleLight &light = SoftwareRenderer::getVisibleLightByID(visLights, lightID);
		const double lightDistSqr =
			((light.position.x - point.x) * (light.position.x - point.x)) +
			((light.position.z - point.y) * (light.position.z - point.y));
		const double lightDist = std::sqrt(lightDistSqr);

		const double val = (light.radius - lightDist) / light.radius;
		lightContributionPercent += std::clamp(val, 0.0, 1.0);

		if constexpr (CappedSum)
		{
			if (lightContributionPercent >= 1.0)
			{
				lightContributionPercent = 1.0;
				break;
			}
		}
	}

	return lightContributionPercent;
}

// @todo: might be better as a macro so there's no chance of a function call in the pixel loop.
template <int FilterMode, bool Transparency>
void SoftwareRenderer::sampleVoxelTexture(const VoxelTexture &texture, double u, double v,
	double *r, double *g, double *b, double *emission, bool *transparent)
{
	const double textureWidthReal = static_cast<double>(texture.width);
	const double textureHeightReal = static_cast<double>(texture.height);

	if constexpr (FilterMode == 0)
	{
		// Nearest.
		const int textureX = static_cast<int>(u * textureWidthReal);
		const int textureY = static_cast<int>(v * textureHeightReal);
		const int textureIndex = textureX + (textureY * texture.width);

		const VoxelTexel &texel = texture.texels[textureIndex];
		*r = texel.r;
		*g = texel.g;
		*b = texel.b;
		*emission = texel.emission;
		
		if constexpr (Transparency)
		{
			*transparent = texel.transparent;
		}
	}
	else if constexpr (FilterMode == 1)
	{
		// Linear.
		const double texelWidth = 1.0 / textureWidthReal;
		const double texelHeight = 1.0 / textureHeightReal;
		const double halfTexelWidth = texelWidth / 2.0;
		const double halfTexelHeight = texelHeight / 2.0;
		const double uL = std::max(u - halfTexelWidth, 0.0); // Change to wrapping for better texture edges
		const double uR = std::min(u + halfTexelWidth, Constants::JustBelowOne);
		const double vT = std::max(v - halfTexelHeight, 0.0);
		const double vB = std::min(v + halfTexelHeight, Constants::JustBelowOne);
		const double uLWidth = uL * textureWidthReal;
		const double vTHeight = vT * textureHeightReal;
		const double uLPercent = 1.0 - (uLWidth - std::floor(uLWidth));
		const double uRPercent = 1.0 - uLPercent;
		const double vTPercent = 1.0 - (vTHeight - std::floor(vTHeight));
		const double vBPercent = 1.0 - vTPercent;
		const double tlPercent = uLPercent * vTPercent;
		const double trPercent = uRPercent * vTPercent;
		const double blPercent = uLPercent * vBPercent;
		const double brPercent = uRPercent * vBPercent;
		const int textureXL = static_cast<int>(uL * textureWidthReal);
		const int textureXR = static_cast<int>(uR * textureWidthReal);
		const int textureYT = static_cast<int>(vT * textureHeightReal);
		const int textureYB = static_cast<int>(vB * textureHeightReal);
		const int textureIndexTL = textureXL + (textureYT * texture.width);
		const int textureIndexTR = textureXR + (textureYT * texture.width);
		const int textureIndexBL = textureXL + (textureYB * texture.width);
		const int textureIndexBR = textureXR + (textureYB * texture.width);

		const VoxelTexel &texelTL = texture.texels[textureIndexTL];
		const VoxelTexel &texelTR = texture.texels[textureIndexTR];
		const VoxelTexel &texelBL = texture.texels[textureIndexBL];
		const VoxelTexel &texelBR = texture.texels[textureIndexBR];
		*r = (texelTL.r * tlPercent) + (texelTR.r * trPercent) + (texelBL.r * blPercent) + (texelBR.r * brPercent);
		*g = (texelTL.g * tlPercent) + (texelTR.g * trPercent) + (texelBL.g * blPercent) + (texelBR.g * brPercent);
		*b = (texelTL.b * tlPercent) + (texelTR.b * trPercent) + (texelBL.b * blPercent) + (texelBR.b * brPercent);
		*emission = (texelTL.emission * tlPercent) + (texelTR.emission * trPercent) +
			(texelBL.emission * blPercent) + (texelBR.emission * brPercent);

		if constexpr (Transparency)
		{
			*transparent = texelTL.transparent && texelTR.transparent &&
				texelBL.transparent && texelBR.transparent;
		}
	}
	else
	{
		// Silently fail; don't want error reporting in a pixel shader.
		// (apparently can't static assert false here on GCC/Clang).
	}
}

void SoftwareRenderer::sampleChasmTexture(const ChasmTexture &texture, double screenXPercent,
	double screenYPercent, double *r, double *g, double *b)
{
	const double textureWidthReal = static_cast<double>(texture.width);
	const double textureHeightReal = static_cast<double>(texture.height);

	// @todo: this is just the first implementation of chasm texturing. There is apparently no
	// perfect solution, so there will probably be graphics options to tweak how exactly this
	// sampling is done (stretch, tile, etc.).
	const int textureX = static_cast<int>(screenXPercent * textureWidthReal);
	const int textureY = static_cast<int>((screenYPercent * 2.0) * textureHeightReal) % texture.height;
	const int textureIndex = textureX + (textureY * texture.width);

	const ChasmTexel &texel = texture.texels[textureIndex];
	*r = texel.r;
	*g = texel.g;
	*b = texel.b;
}

template <bool Fading>
void SoftwareRenderer::drawPixelsShader(int x, const DrawRange &drawRange, double depth,
	double u, double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
	double fadePercent, double lightContributionPercent, const ShadingInfo &shadingInfo,
	OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	// - Taken care of in texture sampling function (redundant calculation, though).
	//const int textureX = static_cast<int>(u * static_cast<double>(texture.width));

	// Linearly interpolated fog.
	const Double3 &fogColor = shadingInfo.getFogColor();
	const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

	// Shading on the texture.
	const Double3 shading(shadingInfo.ambient, shadingInfo.ambient, shadingInfo.ambient);

	// Clip the Y start and end coordinates as needed, and refresh the occlusion buffer.
	occlusion.clipRange(&yStart, &yEnd);
	occlusion.update(yStart, yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Check depth of the pixel before rendering.
		// - @todo: implement occlusion culling and back-to-front transparent rendering so
		//   this depth check isn't needed.
		if (depth <= (frame.depthBuffer[index] - Constants::Epsilon))
		{
			// Percent stepped from beginning to end on the column.
			const double yPercent =
				((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

			// Vertical texture coordinate.
			const double v = vStart + ((vEnd - vStart) * yPercent);

			// Texture color. Alpha is ignored in this loop, so transparent texels will appear black.
			constexpr bool TextureTransparency = false;
			double colorR, colorG, colorB, colorEmission;
			SoftwareRenderer::sampleVoxelTexture<TextureFilterMode, TextureTransparency>(
				texture, u, v, &colorR, &colorG, &colorB, &colorEmission, nullptr);

			// Shading from light.
			constexpr double shadingMax = 1.0;
			const double combinedEmission = colorEmission + lightContributionPercent;
			const double lightR = shading.x + combinedEmission;
			const double lightG = shading.y + combinedEmission;
			const double lightB = shading.z + combinedEmission;
			colorR *= (lightR < shadingMax) ? lightR : shadingMax;
			colorG *= (lightG < shadingMax) ? lightG : shadingMax;
			colorB *= (lightB < shadingMax) ? lightB : shadingMax;

			if constexpr (Fading)
			{
				// Apply voxel fade percent.
				colorR *= fadePercent;
				colorG *= fadePercent;
				colorB *= fadePercent;
			}

			// Linearly interpolate with fog.
			colorR += (fogColor.x - colorR) * fogPercent;
			colorG += (fogColor.y - colorG) * fogPercent;
			colorB += (fogColor.z - colorB) * fogPercent;

			// Clamp maximum (don't worry about negative values).
			constexpr double high = 1.0;
			colorR = (colorR > high) ? high : colorR;
			colorG = (colorG > high) ? high : colorG;
			colorB = (colorB > high) ? high : colorB;

			// Convert floats to integers.
			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;
			frame.depthBuffer[index] = depth;
		}
	}
}

void SoftwareRenderer::drawPixels(int x, const DrawRange &drawRange, double depth, double u,
	double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
	double fadePercent, double lightContributionPercent, const ShadingInfo &shadingInfo,
	OcclusionData &occlusion, const FrameView &frame)
{
	if (fadePercent == 1.0)
	{
		constexpr bool fading = false;
		SoftwareRenderer::drawPixelsShader<fading>(x, drawRange, depth, u, vStart, vEnd, normal, texture,
			fadePercent, lightContributionPercent, shadingInfo, occlusion, frame);
	}
	else
	{
		constexpr bool fading = true;
		SoftwareRenderer::drawPixelsShader<fading>(x, drawRange, depth, u, vStart, vEnd, normal, texture,
			fadePercent, lightContributionPercent, shadingInfo, occlusion, frame);
	}
}

template <bool Fading>
void SoftwareRenderer::drawPerspectivePixelsShader(int x, const DrawRange &drawRange,
	const NewDouble2 &startPoint, const NewDouble2 &endPoint, double depthStart, double depthEnd,
	const Double3 &normal, const VoxelTexture &texture, double fadePercent,
	const BufferView<const VisibleLight> &visLights, const VisibleLightList &visLightList,
	const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Fog color to interpolate with.
	const Double3 &fogColor = shadingInfo.getFogColor();

	// Base shading on the texture.
	const Double3 shading(shadingInfo.ambient, shadingInfo.ambient, shadingInfo.ambient);

	// Values for perspective-correct interpolation.
	const double depthStartRecip = 1.0 / depthStart;
	const double depthEndRecip = 1.0 / depthEnd;
	const NewDouble2 startPointDiv = startPoint * depthStartRecip;
	const NewDouble2 endPointDiv = endPoint * depthEndRecip;
	const NewDouble2 pointDivDiff = endPointDiv - startPointDiv;

	// Clip the Y start and end coordinates as needed, and refresh the occlusion buffer.
	occlusion.clipRange(&yStart, &yEnd);
	occlusion.update(yStart, yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Percent stepped from beginning to end on the column.
		const double yPercent =
			((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

		// Interpolate between the near and far depth.
		const double depth = 1.0 /
			(depthStartRecip + ((depthEndRecip - depthStartRecip) * yPercent));

		// Check depth of the pixel before rendering.
		// - @todo: implement occlusion culling and back-to-front transparent rendering so
		//   this depth check isn't needed.
		if (depth <= frame.depthBuffer[index])
		{
			// Linearly interpolated fog.
			const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

			// Interpolate between start and end points.
			const SNDouble currentPointX = (startPointDiv.x + (pointDivDiff.x * yPercent)) * depth;
			const WEDouble currentPointY = (startPointDiv.y + (pointDivDiff.y * yPercent)) * depth;

			// Texture coordinates.
			const double u = std::clamp(currentPointX - std::floor(currentPointX), 0.0, Constants::JustBelowOne);
			const double v = std::clamp(currentPointY - std::floor(currentPointY), 0.0, Constants::JustBelowOne);

			// Texture color. Alpha is ignored in this loop, so transparent texels will appear black.
			constexpr bool TextureTransparency = false;
			double colorR, colorG, colorB, colorEmission;
			SoftwareRenderer::sampleVoxelTexture<TextureFilterMode, TextureTransparency>(
				texture, u, v, &colorR, &colorG, &colorB, &colorEmission, nullptr);

			// Light contribution.
			const NewDouble2 currentPoint(currentPointX, currentPointY);
			const double lightContributionPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(currentPoint, visLights, visLightList);

			// Shading from light.
			constexpr double shadingMax = 1.0;
			const double combinedEmission = colorEmission + lightContributionPercent;
			const double lightR = shading.x + combinedEmission;
			const double lightG = shading.y + combinedEmission;
			const double lightB = shading.z + combinedEmission;
			colorR *= (lightR < shadingMax) ? lightR : shadingMax;
			colorG *= (lightG < shadingMax) ? lightG : shadingMax;
			colorB *= (lightB < shadingMax) ? lightB : shadingMax;

			if constexpr (Fading)
			{
				// Apply voxel fade percent.
				colorR *= fadePercent;
				colorG *= fadePercent;
				colorB *= fadePercent;
			}

			// Linearly interpolate with fog.
			colorR += (fogColor.x - colorR) * fogPercent;
			colorG += (fogColor.y - colorG) * fogPercent;
			colorB += (fogColor.z - colorB) * fogPercent;

			// Clamp maximum (don't worry about negative values).
			constexpr double high = 1.0;
			colorR = (colorR > high) ? high : colorR;
			colorG = (colorG > high) ? high : colorG;
			colorB = (colorB > high) ? high : colorB;

			// Convert floats to integers.
			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;
			frame.depthBuffer[index] = depth;
		}
	}
}

void SoftwareRenderer::drawPerspectivePixels(int x, const DrawRange &drawRange,
	const NewDouble2 &startPoint, const NewDouble2 &endPoint, double depthStart, double depthEnd,
	const Double3 &normal, const VoxelTexture &texture, double fadePercent,
	const BufferView<const VisibleLight> &visLights, const VisibleLightList &visLightList,
	const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame)
{
	if (fadePercent == 1.0)
	{
		constexpr bool fading = false;
		SoftwareRenderer::drawPerspectivePixelsShader<fading>(x, drawRange, startPoint, endPoint,
			depthStart, depthEnd, normal, texture, fadePercent, visLights, visLightList,
			shadingInfo, occlusion, frame);
	}
	else
	{
		constexpr bool fading = true;
		SoftwareRenderer::drawPerspectivePixelsShader<fading>(x, drawRange, startPoint, endPoint,
			depthStart, depthEnd, normal, texture, fadePercent, visLights, visLightList,
			shadingInfo, occlusion, frame);
	}
}

void SoftwareRenderer::drawTransparentPixels(int x, const DrawRange &drawRange, double depth,
	double u, double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
	double lightContributionPercent, const ShadingInfo &shadingInfo,
	const OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	// - Taken care of in texture sampling function (redundant calculation, though).
	//const int textureX = static_cast<int>(u * static_cast<double>(texture.width));

	// Linearly interpolated fog.
	const Double3 &fogColor = shadingInfo.getFogColor();
	const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

	// Shading on the texture.
	const Double3 shading(shadingInfo.ambient, shadingInfo.ambient, shadingInfo.ambient);

	// Clip the Y start and end coordinates as needed, but do not refresh the occlusion buffer,
	// because transparent ranges do not occlude as simply as opaque ranges.
	occlusion.clipRange(&yStart, &yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Check depth of the pixel before rendering.
		if (depth <= (frame.depthBuffer[index] - Constants::Epsilon))
		{
			// Percent stepped from beginning to end on the column.
			const double yPercent =
				((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

			// Vertical texture coordinate.
			const double v = vStart + ((vEnd - vStart) * yPercent);

			// Texture color. Alpha is checked in this loop, and transparent texels are not drawn.
			constexpr bool TextureTransparency = true;
			double colorR, colorG, colorB, colorEmission;
			bool colorTransparent;
			SoftwareRenderer::sampleVoxelTexture<TextureFilterMode, TextureTransparency>(
				texture, u, v, &colorR, &colorG, &colorB, &colorEmission, &colorTransparent);
			
			if (!colorTransparent)
			{
				// Shading from light.
				constexpr double shadingMax = 1.0;
				const double combinedEmission = colorEmission + lightContributionPercent;
				const double lightR = shading.x + combinedEmission;
				const double lightG = shading.y + combinedEmission;
				const double lightB = shading.z + combinedEmission;
				colorR *= (lightR < shadingMax) ? lightR : shadingMax;
				colorG *= (lightG < shadingMax) ? lightG : shadingMax;
				colorB *= (lightB < shadingMax) ? lightB : shadingMax;

				// Linearly interpolate with fog.
				colorR += (fogColor.x - colorR) * fogPercent;
				colorG += (fogColor.y - colorG) * fogPercent;
				colorB += (fogColor.z - colorB) * fogPercent;
				
				// Clamp maximum (don't worry about negative values).
				constexpr double high = 1.0;
				colorR = (colorR > high) ? high : colorR;
				colorG = (colorG > high) ? high : colorG;
				colorB = (colorB > high) ? high : colorB;

				// Convert floats to integers.
				const uint32_t colorRGB = static_cast<uint32_t>(
					((static_cast<uint8_t>(colorR * 255.0)) << 16) |
					((static_cast<uint8_t>(colorG * 255.0)) << 8) |
					((static_cast<uint8_t>(colorB * 255.0))));

				frame.colorBuffer[index] = colorRGB;
				frame.depthBuffer[index] = depth;
			}
		}
	}
}

template <bool AmbientShading, bool TrueDepth>
void SoftwareRenderer::drawChasmPixelsShader(int x, const DrawRange &drawRange, double depth,
	double u, double vStart, double vEnd, const Double3 &normal, const VoxelTexture &texture,
	const ChasmTexture &chasmTexture, double lightContributionPercent, const ShadingInfo &shadingInfo,
	OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	// - Taken care of in texture sampling function (redundant calculation, though).
	//const int textureX = static_cast<int>(u * static_cast<double>(texture.width));

	// Linearly interpolated fog.
	const Double3 &fogColor = shadingInfo.getFogColor();
	const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

	// Shading on the texture.
	const Double3 shading(shadingInfo.ambient, shadingInfo.ambient, shadingInfo.ambient);

	// Clip the Y start and end coordinates as needed, and refresh the occlusion buffer.
	occlusion.clipRange(&yStart, &yEnd);
	occlusion.update(yStart, yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Check depth of the pixel before rendering.
		if (depth <= (frame.depthBuffer[index] - Constants::Epsilon))
		{
			// Percent stepped from beginning to end on the column.
			const double yPercent =
				((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

			// Vertical texture coordinate.
			const double v = vStart + ((vEnd - vStart) * yPercent);

			// Texture color. If the texel is transparent, use the chasm texture instead.
			// @todo: maybe this could be optimized to a 'transparent-texel-only' look-up, that
			// then branches to determine whether to sample the voxel or chasm texture?
			constexpr bool TextureTransparency = true;
			double colorR, colorG, colorB, colorEmission;
			bool colorTransparent;
			SoftwareRenderer::sampleVoxelTexture<TextureFilterMode, TextureTransparency>(
				texture, u, v, &colorR, &colorG, &colorB, &colorEmission, &colorTransparent);

			if (!colorTransparent)
			{
				// Voxel texture.
				// Shading from light.
				constexpr double shadingMax = 1.0;
				const double combinedEmission = colorEmission + lightContributionPercent;
				const double lightR = shading.x + combinedEmission;
				const double lightG = shading.y + combinedEmission;
				const double lightB = shading.z + combinedEmission;
				colorR *= (lightR < shadingMax) ? lightR : shadingMax;
				colorG *= (lightG < shadingMax) ? lightG : shadingMax;
				colorB *= (lightB < shadingMax) ? lightB : shadingMax;

				// Linearly interpolate with fog.
				colorR += (fogColor.x - colorR) * fogPercent;
				colorG += (fogColor.y - colorG) * fogPercent;
				colorB += (fogColor.z - colorB) * fogPercent;

				// Clamp maximum (don't worry about negative values).
				constexpr double high = 1.0;
				colorR = (colorR > high) ? high : colorR;
				colorG = (colorG > high) ? high : colorG;
				colorB = (colorB > high) ? high : colorB;

				// Convert floats to integers.
				const uint32_t colorRGB = static_cast<uint32_t>(
					((static_cast<uint8_t>(colorR * 255.0)) << 16) |
					((static_cast<uint8_t>(colorG * 255.0)) << 8) |
					((static_cast<uint8_t>(colorB * 255.0))));

				frame.colorBuffer[index] = colorRGB;
				frame.depthBuffer[index] = depth;
			}
			else
			{
				// Chasm texture.
				const double screenXPercent = static_cast<double>(x) / frame.widthReal;
				const double screenYPercent = static_cast<double>(y) / frame.heightReal;
				double chasmR, chasmG, chasmB;
				SoftwareRenderer::sampleChasmTexture(chasmTexture, screenXPercent, screenYPercent,
					&chasmR, &chasmG, &chasmB);

				if constexpr (AmbientShading)
				{
					chasmR *= shadingInfo.distantAmbient;
					chasmG *= shadingInfo.distantAmbient;
					chasmB *= shadingInfo.distantAmbient;
				}

				const uint32_t colorRGB = static_cast<uint32_t>(
					((static_cast<uint8_t>(chasmR * 255.0)) << 16) |
					((static_cast<uint8_t>(chasmG * 255.0)) << 8) |
					((static_cast<uint8_t>(chasmB * 255.0))));

				frame.colorBuffer[index] = colorRGB;

				if constexpr (TrueDepth)
				{
					frame.depthBuffer[index] = depth;
				}
				else
				{
					frame.depthBuffer[index] = DEPTH_BUFFER_INFINITY;
				}
			}
		}
	}
}

void SoftwareRenderer::drawChasmPixels(int x, const DrawRange &drawRange, double depth, double u,
	double vStart, double vEnd, const Double3 &normal, bool emissive, const VoxelTexture &texture,
	const ChasmTexture &chasmTexture, double lightContributionPercent, const ShadingInfo &shadingInfo,
	OcclusionData &occlusion, const FrameView &frame)
{
	const bool useAmbientChasmShading = shadingInfo.isExterior && !emissive;
	const bool useTrueChasmDepth = true;

	if (useAmbientChasmShading)
	{
		constexpr bool ambientShading = true;
		if (useTrueChasmDepth)
		{
			constexpr bool trueDepth = true;
			SoftwareRenderer::drawChasmPixelsShader<ambientShading, trueDepth>(x, drawRange, depth,
				u, vStart, vEnd, normal, texture, chasmTexture, lightContributionPercent, shadingInfo,
				occlusion, frame);
		}
		else
		{
			constexpr bool trueDepth = false;
			SoftwareRenderer::drawChasmPixelsShader<ambientShading, trueDepth>(x, drawRange, depth,
				u, vStart, vEnd, normal, texture, chasmTexture, lightContributionPercent, shadingInfo,
				occlusion, frame);
		}
	}
	else
	{
		constexpr bool ambientShading = false;
		if (useTrueChasmDepth)
		{
			constexpr bool trueDepth = true;
			SoftwareRenderer::drawChasmPixelsShader<ambientShading, trueDepth>(x, drawRange, depth,
				u, vStart, vEnd, normal, texture, chasmTexture, lightContributionPercent, shadingInfo,
				occlusion, frame);
		}
		else
		{
			constexpr bool trueDepth = false;
			SoftwareRenderer::drawChasmPixelsShader<ambientShading, trueDepth>(x, drawRange, depth,
				u, vStart, vEnd, normal, texture, chasmTexture, lightContributionPercent, shadingInfo,
				occlusion, frame);
		}
	}
}

template <bool AmbientShading, bool TrueDepth>
void SoftwareRenderer::drawPerspectiveChasmPixelsShader(int x, const DrawRange &drawRange,
	const NewDouble2 &startPoint, const NewDouble2 &endPoint, double depthStart, double depthEnd,
	const Double3 &normal, const ChasmTexture &texture, const ShadingInfo &shadingInfo,
	OcclusionData &occlusion, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	int yStart = drawRange.yStart;
	int yEnd = drawRange.yEnd;

	// Fog color to interpolate with.
	const Double3 &fogColor = shadingInfo.getFogColor();

	// Shading on the texture.
	const Double3 shading(shadingInfo.ambient, shadingInfo.ambient, shadingInfo.ambient);

	// Values for perspective-correct interpolation.
	const double depthStartRecip = 1.0 / depthStart;
	const double depthEndRecip = 1.0 / depthEnd;
	const NewDouble2 startPointDiv = startPoint * depthStartRecip;
	const NewDouble2 endPointDiv = endPoint * depthEndRecip;
	const NewDouble2 pointDivDiff = endPointDiv - startPointDiv;

	// Clip the Y start and end coordinates as needed, and refresh the occlusion buffer.
	occlusion.clipRange(&yStart, &yEnd);
	occlusion.update(yStart, yEnd);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Percent stepped from beginning to end on the column.
		const double yPercent =
			((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

		// Interpolate between the near and far depth.
		const double depth = 1.0 /
			(depthStartRecip + ((depthEndRecip - depthStartRecip) * yPercent));

		// Check depth of the pixel before rendering.
		// - @todo: implement occlusion culling and back-to-front transparent rendering so
		//   this depth check isn't needed.
		if (depth <= frame.depthBuffer[index])
		{
			// Linearly interpolated fog.
			const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

			// Interpolate between start and end points.
			const SNDouble currentPointX = (startPointDiv.x + (pointDivDiff.x * yPercent)) * depth;
			const WEDouble currentPointY = (startPointDiv.y + (pointDivDiff.y * yPercent)) * depth;

			// Texture coordinates.
			const double u = std::clamp(currentPointX - std::floor(currentPointX), 0.0, Constants::JustBelowOne);
			const double v = std::clamp(currentPointY - std::floor(currentPointY), 0.0, Constants::JustBelowOne);

			// Chasm texture color.
			const double screenXPercent = static_cast<double>(x) / frame.widthReal;
			const double screenYPercent = static_cast<double>(y) / frame.heightReal;
			double colorR, colorG, colorB;
			SoftwareRenderer::sampleChasmTexture(texture, screenXPercent, screenYPercent,
				&colorR, &colorG, &colorB);

			if constexpr (AmbientShading)
			{
				colorR *= shadingInfo.distantAmbient;
				colorG *= shadingInfo.distantAmbient;
				colorB *= shadingInfo.distantAmbient;
			}

			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;

			if constexpr (TrueDepth)
			{
				frame.depthBuffer[index] = depth;
			}
			else
			{
				frame.depthBuffer[index] = DEPTH_BUFFER_INFINITY;
			}
		}
	}
}

void SoftwareRenderer::drawPerspectiveChasmPixels(int x, const DrawRange &drawRange,
	const NewDouble2 &startPoint, const NewDouble2 &endPoint, double depthStart, double depthEnd,
	const Double3 &normal, bool emissive, const ChasmTexture &texture,
	const ShadingInfo &shadingInfo, OcclusionData &occlusion, const FrameView &frame)
{
	const bool useAmbientChasmShading = shadingInfo.isExterior && !emissive;
	const bool useTrueChasmDepth = true;

	if (useAmbientChasmShading)
	{
		constexpr bool ambientShading = true;
		if (useTrueChasmDepth)
		{
			constexpr bool trueDepth = true;
			SoftwareRenderer::drawPerspectiveChasmPixelsShader<ambientShading, trueDepth>(
				x, drawRange, startPoint, endPoint, depthStart, depthEnd, normal, texture,
				shadingInfo, occlusion, frame);
		}
		else
		{
			constexpr bool trueDepth = false;
			SoftwareRenderer::drawPerspectiveChasmPixelsShader<ambientShading, trueDepth>(
				x, drawRange, startPoint, endPoint, depthStart, depthEnd, normal, texture,
				shadingInfo, occlusion, frame);
		}
	}
	else
	{
		constexpr bool ambientShading = false;
		if (useTrueChasmDepth)
		{
			constexpr bool trueDepth = true;
			SoftwareRenderer::drawPerspectiveChasmPixelsShader<ambientShading, trueDepth>(
				x, drawRange, startPoint, endPoint, depthStart, depthEnd, normal, texture,
				shadingInfo, occlusion, frame);
		}
		else
		{
			constexpr bool trueDepth = false;
			SoftwareRenderer::drawPerspectiveChasmPixelsShader<ambientShading, trueDepth>(
				x, drawRange, startPoint, endPoint, depthStart, depthEnd, normal, texture,
				shadingInfo, occlusion, frame);
		}
	}
}

void SoftwareRenderer::drawDistantPixels(int x, const DrawRange &drawRange, double u,
	double vStart, double vEnd, const SkyTexture &texture, bool emissive,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	const int yStart = drawRange.yStart;
	const int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(texture.width));
	
	// Shading on the texture. Some distant objects are completely bright.
	const double shading = emissive ? 1.0 : shadingInfo.distantAmbient;

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Percent stepped from beginning to end on the column.
		const double yPercent =
			((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

		// Vertical texture coordinate.
		const double v = vStart + ((vEnd - vStart) * yPercent);

		// Y position in texture.
		const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

		// Alpha is checked in this loop, and transparent texels are not drawn.
		const int textureIndex = textureX + (textureY * texture.width);
		const SkyTexel &texel = texture.texels[textureIndex];

		if (texel.a != 0.0)
		{
			// Special case (for true color): if texel alpha is between 0 and 1,
			// the previously rendered pixel is diminished by some amount. This is mostly
			// only pertinent to the edges of some clouds (with respect to distant sky).
			double colorR, colorG, colorB;
			if (texel.a < 1.0)
			{
				// Diminish the previous color in the frame buffer.
				const Double3 prevColor = Double3::fromRGB(frame.colorBuffer[index]);
				const double visPercent = std::clamp(1.0 - texel.a, 0.0, 1.0);
				colorR = prevColor.x * visPercent;
				colorG = prevColor.y * visPercent;
				colorB = prevColor.z * visPercent;
			}
			else
			{
				// Texture color with shading.
				colorR = texel.r * shading;
				colorG = texel.g * shading;
				colorB = texel.b * shading;
			}

			// Clamp maximum (don't worry about negative values).
			const double high = 1.0;
			colorR = (colorR > high) ? high : colorR;
			colorG = (colorG > high) ? high : colorG;
			colorB = (colorB > high) ? high : colorB;

			// Convert floats to integers.
			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;
		}
	}
}

/*void SoftwareRenderer::drawDistantPixelsSSE(int x, const DrawRange &drawRange, double u,
	double vStart, double vEnd, const SkyTexture &texture, bool emissive,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// @todo: fix. this is now out of sync with the non-SSE version.

	// Draw range values.
	const int yStart = drawRange.yStart;
	const int yEnd = drawRange.yEnd;
	const __m128d yProjStarts = _mm_set1_pd(drawRange.yProjStart);
	const __m128d yProjEnds = _mm_set1_pd(drawRange.yProjEnd);
	const __m128d yStarts = _mm_set1_pd(drawRange.yStart);
	const __m128d yEnds = _mm_set1_pd(drawRange.yEnd);

	// Horizontal offset in texture.
	const __m128i textureXs = [u, &texture]()
	{
		const __m128d us = _mm_set1_pd(std::min(u, Constants::JustBelowOne));
		const __m128d textureWidths = _mm_cvtepi32_pd(_mm_set1_epi32(texture.width));
		const __m128d mults = _mm_mul_pd(us, textureWidths);
		return _mm_cvttpd_epi32(mults);
	}();

	// Shading on the texture. Some distant objects are completely bright.
	const __m128d shadings = _mm_set1_pd(emissive ? 1.0 : shadingInfo.distantAmbient);

	// Some pre-calculated values.
	const __m128i zeroes = _mm_set1_epi32(0);
	const __m128d halfReals = _mm_set1_pd(0.50);
	const __m128d oneReals = _mm_set1_pd(1.0);
	const __m128d twoFiftyFiveReals = _mm_set1_pd(255.0);
	const __m128i xs = _mm_set1_epi32(x);
	const __m128i frameWidths = _mm_set1_epi32(frame.width);
	const __m128i frameHeights = _mm_set1_epi32(frame.height);
	const __m128d yProjDiffs = _mm_sub_pd(yProjStarts, yProjEnds);
	const __m128d vStarts = _mm_set1_pd(std::max(vStart, 0.0));
	const __m128d vEnds = _mm_set1_pd(std::min(vEnd, Constants::JustBelowOne));
	const __m128d vDiffs = _mm_sub_pd(vStarts, vEnds);
	const __m128i textureWidths = _mm_set1_epi32(texture.width);
	const __m128i textureHeights = _mm_set1_epi32(texture.height);
	const __m128d textureWidthReals = _mm_cvtepi32_pd(textureWidths);
	const __m128d textureHeightReals = _mm_cvtepi32_pd(textureHeights);

	// SIMD stride size.
	constexpr int stride = sizeof(__m128d) / sizeof(double);
	static_assert(stride == 2);

	// @todo: need special case loop afterwards to catch missed rows.
	for (int y = yStart; y < (yEnd - (stride - 1)); y += stride)
	{
		// Row and frame buffer index.
		const __m128i ys = _mm_setr_epi32(y, y + 1, 0, 0);
		const __m128i yRowOffsets = _mm_mullo_epi32(ys, frameWidths);

		// Percents stepped from beginning to end on the column.
		const __m128d yReals = _mm_cvtepi32_pd(ys);
		const __m128d yMidpoints = _mm_add_pd(yReals, halfReals);
		const __m128d yMidpointDiffs = _mm_sub_pd(yMidpoints, yProjStarts);
		const __m128d yPercents = _mm_div_pd(yMidpointDiffs, yProjDiffs);

		// Vertical texture coordinate.
		const __m128d vDiffYPercents = _mm_mul_pd(vDiffs, yPercents);
		const __m128d vs = _mm_add_pd(vStarts, vDiffYPercents);

		// Y position in texture.
		const __m128d vTextureHeights = _mm_mul_pd(vs, textureHeightReals);
		const __m128i textureYs = _mm_cvttpd_epi32(vTextureHeights);

		// Alpha is checked in this loop, and transparent texels are not drawn.
		const __m128i textureYWidths = _mm_mullo_epi32(textureYs, textureWidths);
		const __m128i textureIndices = _mm_add_epi32(textureXs, textureYWidths);

		// Load alpha component of texel to see if it's even visible.
		// @todo: proper SIMD texel format.
		// - For now, assume we can load texels really fast?! No gather instruction for SSE.
		// - This seems like a big bandwidth win once each color channel is in its own array.
		const SkyTexel &texel0 = texture.texels[_mm_extract_epi32(textureIndices, 0)];
		const SkyTexel &texel1 = texture.texels[_mm_extract_epi32(textureIndices, 1)];
		const __m128i texelAs = _mm_setr_epi32(
			static_cast<int>(texel0.a == 0.0),
			static_cast<int>(texel1.a == 0.0),
			static_cast<int>(false),
			static_cast<int>(false));

		// Check if the texel is opaque.
		const __m128i opaques = _mm_cmpeq_epi32(texelAs, zeroes);
		const bool opaque0 = _mm_extract_epi32(opaques, 0) != 0;
		const bool opaque1 = _mm_extract_epi32(opaques, 0) != 0;
		const bool anyOpaque = opaque0 || opaque1;
		if (anyOpaque)
		{
			// @todo: missing transparency branch of non-SSE version.
			// Texel colors.
			const __m128d texelRs = _mm_setr_pd(texel0.r, texel1.r);
			const __m128d texelGs = _mm_setr_pd(texel0.g, texel1.g);
			const __m128d texelBs = _mm_setr_pd(texel0.b, texel1.b);

			// Texture color with shading.
			__m128d colorRs = _mm_mul_pd(texelRs, shadings);
			__m128d colorGs = _mm_mul_pd(texelGs, shadings);
			__m128d colorBs = _mm_mul_pd(texelBs, shadings);

			// Clamp maximum (don't worry about negative values).
			const __m128d highs = oneReals;
			__m128d colorRCmps = _mm_cmpgt_pd(colorRs, highs);
			__m128d colorGCmps = _mm_cmpgt_pd(colorGs, highs);
			__m128d colorBCmps = _mm_cmpgt_pd(colorBs, highs);
			colorRs = _mm_blendv_pd(colorRs, highs, colorRCmps);
			colorGs = _mm_blendv_pd(colorGs, highs, colorGCmps);
			colorBs = _mm_blendv_pd(colorBs, highs, colorBCmps);

			// Convert floats to integers.
			const __m128d mul255s = twoFiftyFiveReals;
			const __m128d colorRs255 = _mm_mul_pd(colorRs, mul255s);
			const __m128d colorGs255 = _mm_mul_pd(colorGs, mul255s);
			const __m128d colorBs255 = _mm_mul_pd(colorBs, mul255s);
			const __m128i colorRsU32 = _mm_cvttpd_epi32(colorRs255);
			const __m128i colorGsU32 = _mm_cvttpd_epi32(colorGs255);
			const __m128i colorBsU32 = _mm_cvttpd_epi32(colorBs255);
			const __m128i colorRsShifted = _mm_slli_epi32(colorRsU32, 16);
			const __m128i colorGsShifted = _mm_slli_epi32(colorGsU32, 8);
			const __m128i colorBsShifted = colorBsU32;
			const __m128i colors = _mm_or_si128(
				_mm_or_si128(colorRsShifted, colorGsShifted), colorBsShifted);

			// Frame buffer index.
			const __m128i indices = _mm_add_epi32(xs, yRowOffsets);

			// (SIMD only) Conditionally write pixels based on texel opacity.
			if (opaque0)
			{
				const int index0 = _mm_extract_epi32(indices, 0);
				const uint32_t color0 = _mm_extract_epi32(colors, 0);
				frame.colorBuffer[index0] = color0;
			}

			if (opaque1)
			{
				const int index1 = _mm_extract_epi32(indices, 1);
				const uint32_t color1 = _mm_extract_epi32(colors, 1);
				frame.colorBuffer[index1] = color1;
			}
		}
	}
}*/

/*void SoftwareRenderer::drawDistantPixelsAVX(int x, const DrawRange &drawRange, double u,
	double vStart, double vEnd, const SkyTexture &texture, bool emissive,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// Draw range values.
	const int yStart = drawRange.yStart;
	const int yEnd = drawRange.yEnd;
	const __m256d yProjStarts = _mm256_set1_pd(drawRange.yProjStart);
	const __m256d yProjEnds = _mm256_set1_pd(drawRange.yProjEnd);
	const __m256d yStarts = _mm256_set1_pd(drawRange.yStart);
	const __m256d yEnds = _mm256_set1_pd(drawRange.yEnd);

	// Horizontal offset in texture.
	const __m128i textureXs = [u, &texture]()
	{
		const __m256d us = _mm256_set1_pd(std::min(u, Constants::JustBelowOne));
		const __m256d textureWidths = _mm256_cvtepi32_pd(_mm_set1_epi32(texture.width));
		const __m256d mults = _mm256_mul_pd(us, textureWidths);
		return _mm256_cvttpd_epi32(mults);
	}();

	// Shading on the texture. Some distant objects are completely bright.
	const __m256d shadings = _mm256_set1_pd(emissive ? 1.0 : shadingInfo.distantAmbient);

	// Some pre-calculated values.
	const __m128i zeroes = _mm_set1_epi32(0);
	const __m256d halfReals = _mm256_set1_pd(0.50);
	const __m256d oneReals = _mm256_set1_pd(1.0);
	const __m256d twoFiftyFiveReals = _mm256_set1_pd(255.0);
	const __m128i xs = _mm_set1_epi32(x);
	const __m128i frameWidths = _mm_set1_epi32(frame.width);
	const __m128i frameHeights = _mm_set1_epi32(frame.height);
	const __m256d yProjDiffs = _mm256_sub_pd(yProjStarts, yProjEnds);
	const __m256d vStarts = _mm256_set1_pd(std::max(vStart, 0.0));
	const __m256d vEnds = _mm256_set1_pd(std::min(vEnd, Constants::JustBelowOne));
	const __m256d vDiffs = _mm256_sub_pd(vStarts, vEnds);
	const __m128i textureWidths = _mm_set1_epi32(texture.width);
	const __m128i textureHeights = _mm_set1_epi32(texture.height);
	const __m256d textureWidthReals = _mm256_cvtepi32_pd(textureWidths);
	const __m256d textureHeightReals = _mm256_cvtepi32_pd(textureHeights);

	// SIMD stride size.
	constexpr int stride = sizeof(__m256d) / sizeof(double);
	static_assert(stride == 4);

	// @todo: need special case loop afterwards to catch missed rows.
	for (int y = yStart; y < (yEnd - (stride - 1)); y += stride)
	{
		// Row and frame buffer index.
		const __m128i ys = _mm_setr_epi32(y, y + 1, y + 2, y + 3);
		const __m128i yRowOffsets = _mm_mullo_epi32(ys, frameWidths);

		// Percents stepped from beginning to end on the column.
		const __m256d yReals = _mm256_cvtepi32_pd(ys);
		const __m256d yMidpoints = _mm256_add_pd(yReals, halfReals);
		const __m256d yMidpointDiffs = _mm256_sub_pd(yMidpoints, yProjStarts);
		const __m256d yPercents = _mm256_div_pd(yMidpointDiffs, yProjDiffs);

		// Vertical texture coordinate.
		const __m256d vDiffYPercents = _mm256_mul_pd(vDiffs, yPercents);
		const __m256d vs = _mm256_add_pd(vStarts, vDiffYPercents);

		// Y position in texture.
		const __m256d vTextureHeights = _mm256_mul_pd(vs, textureHeightReals);
		const __m128i textureYs = _mm256_cvttpd_epi32(vTextureHeights);

		// Alpha is checked in this loop, and transparent texels are not drawn.
		const __m128i textureYWidths = _mm_mullo_epi32(textureYs, textureWidths);
		const __m128i textureIndices = _mm_add_epi32(textureXs, textureYWidths);

		// Load alpha component of texel to see if it's even visible.
		// @todo: proper SIMD texel format.
		// - For now, assume we can load texels really fast?! No gather instruction for SSE.
		// - This seems like a big bandwidth win once each color channel is in its own array.
		const SkyTexel &texel0 = texture.texels[_mm_extract_epi32(textureIndices, 0)];
		const SkyTexel &texel1 = texture.texels[_mm_extract_epi32(textureIndices, 1)];
		const SkyTexel &texel2 = texture.texels[_mm_extract_epi32(textureIndices, 2)];
		const SkyTexel &texel3 = texture.texels[_mm_extract_epi32(textureIndices, 3)];
		const __m128i texelAs = _mm_setr_epi32(
			static_cast<int>(texel0.transparent),
			static_cast<int>(texel1.transparent),
			static_cast<int>(texel2.transparent),
			static_cast<int>(texel3.transparent));

		// Check if the texel is opaque.
		const __m128i opaques = _mm_cmpeq_epi32(texelAs, zeroes);
		const bool opaque0 = _mm_extract_epi32(opaques, 0) != 0;
		const bool opaque1 = _mm_extract_epi32(opaques, 1) != 0;
		const bool opaque2 = _mm_extract_epi32(opaques, 2) != 0;
		const bool opaque3 = _mm_extract_epi32(opaques, 3) != 0;
		const bool anyOpaque = opaque0 || opaque1 || opaque2 || opaque3;
		if (anyOpaque)
		{
			// Texel colors.
			const __m256d texelRs = _mm256_setr_pd(texel0.r, texel1.r, texel2.r, texel3.r);
			const __m256d texelGs = _mm256_setr_pd(texel0.g, texel1.g, texel2.g, texel3.g);
			const __m256d texelBs = _mm256_setr_pd(texel0.b, texel1.b, texel2.b, texel3.b);

			// Texture color with shading.
			__m256d colorRs = _mm256_mul_pd(texelRs, shadings);
			__m256d colorGs = _mm256_mul_pd(texelGs, shadings);
			__m256d colorBs = _mm256_mul_pd(texelBs, shadings);

			// Clamp maximum (don't worry about negative values).
			const __m256d highs = oneReals;
			__m256d colorRCmps = _mm256_cmp_pd(colorRs, highs, _CMP_GT_OS);
			__m256d colorGCmps = _mm256_cmp_pd(colorGs, highs, _CMP_GT_OS);
			__m256d colorBCmps = _mm256_cmp_pd(colorBs, highs, _CMP_GT_OS);
			colorRs = _mm256_blendv_pd(colorRs, highs, colorRCmps);
			colorGs = _mm256_blendv_pd(colorGs, highs, colorGCmps);
			colorBs = _mm256_blendv_pd(colorBs, highs, colorBCmps);

			// Convert floats to integers.
			const __m256d mul255s = twoFiftyFiveReals;
			const __m256d colorRs255 = _mm256_mul_pd(colorRs, mul255s);
			const __m256d colorGs255 = _mm256_mul_pd(colorGs, mul255s);
			const __m256d colorBs255 = _mm256_mul_pd(colorBs, mul255s);
			const __m128i colorRsU32 = _mm256_cvttpd_epi32(colorRs255);
			const __m128i colorGsU32 = _mm256_cvttpd_epi32(colorGs255);
			const __m128i colorBsU32 = _mm256_cvttpd_epi32(colorBs255);
			const __m128i colorRsShifted = _mm_slli_epi32(colorRsU32, 16);
			const __m128i colorGsShifted = _mm_slli_epi32(colorGsU32, 8);
			const __m128i colorBsShifted = colorBsU32;
			const __m128i colors = _mm_or_si128(
				_mm_or_si128(colorRsShifted, colorGsShifted), colorBsShifted);

			// Frame buffer index.
			const __m128i indices = _mm_add_epi32(xs, yRowOffsets);

			// (SIMD only) Conditionally write pixels based on texel opacity.
			if (opaque0)
			{
				const int index0 = _mm_extract_epi32(indices, 0);
				const uint32_t color0 = _mm_extract_epi32(colors, 0);
				frame.colorBuffer[index0] = color0;
			}

			if (opaque1)
			{
				const int index1 = _mm_extract_epi32(indices, 1);
				const uint32_t color1 = _mm_extract_epi32(colors, 1);
				frame.colorBuffer[index1] = color1;
			}

			if (opaque2)
			{
				const int index2 = _mm_extract_epi32(indices, 2);
				const uint32_t color2 = _mm_extract_epi32(colors, 2);
				frame.colorBuffer[index2] = color2;
			}

			if (opaque3)
			{
				const int index3 = _mm_extract_epi32(indices, 3);
				const uint32_t color3 = _mm_extract_epi32(colors, 3);
				frame.colorBuffer[index3] = color3;
			}
		}
	}
}*/

void SoftwareRenderer::drawMoonPixels(int x, const DrawRange &drawRange, double u, double vStart,
	double vEnd, const SkyTexture &texture, const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	const int yStart = drawRange.yStart;
	const int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(texture.width));

	// The gradient color is used for "unlit" texels on the moon's texture.
	constexpr double gradientPercent = 0.80;
	const Double3 gradientColor = SoftwareRenderer::getSkyGradientRowColor(
		gradientPercent, shadingInfo);

	// The 'signal' color used in the original game to denote moon texels that should
	// use the gradient color behind the moon instead.
	const Double3 unlitColor(170.0 / 255.0, 0.0, 0.0);

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Percent stepped from beginning to end on the column.
		const double yPercent =
			((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

		// Vertical texture coordinate.
		const double v = vStart + ((vEnd - vStart) * yPercent);

		// Y position in texture.
		const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

		// Alpha is checked in this loop, and transparent texels are not drawn.
		const int textureIndex = textureX + (textureY * texture.width);
		const SkyTexel &texel = texture.texels[textureIndex];

		if (texel.a != 0.0)
		{
			// Determine how the pixel should be shaded based on the moon texel. Should be
			// safe to do floating-point comparisons here with no error.
			const bool texelIsLit = (texel.r != unlitColor.x) && (texel.g != unlitColor.y) &&
				(texel.b != unlitColor.z);

			double colorR;
			double colorG;
			double colorB;

			if (texelIsLit)
			{
				// Use the moon texel.
				colorR = texel.r;
				colorG = texel.g;
				colorB = texel.b;
			}
			else
			{
				// Use the gradient color.
				colorR = gradientColor.x;
				colorG = gradientColor.y;
				colorB = gradientColor.z;
			}

			// Clamp maximum (don't worry about negative values).
			const double high = 1.0;
			colorR = (colorR > high) ? high : colorR;
			colorG = (colorG > high) ? high : colorG;
			colorB = (colorB > high) ? high : colorB;

			// Convert floats to integers.
			const uint32_t colorRGB = static_cast<uint32_t>(
				((static_cast<uint8_t>(colorR * 255.0)) << 16) |
				((static_cast<uint8_t>(colorG * 255.0)) << 8) |
				((static_cast<uint8_t>(colorB * 255.0))));

			frame.colorBuffer[index] = colorRGB;
		}
	}
}

void SoftwareRenderer::drawStarPixels(int x, const DrawRange &drawRange, double u, double vStart,
	double vEnd, const SkyTexture &texture, const Buffer<Double3> &skyGradientRowCache,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// Draw range values.
	const double yProjStart = drawRange.yProjStart;
	const double yProjEnd = drawRange.yProjEnd;
	const int yStart = drawRange.yStart;
	const int yEnd = drawRange.yEnd;

	// Horizontal offset in texture.
	const int textureX = static_cast<int>(u * static_cast<double>(texture.width));

	// Draw the column to the output buffer.
	for (int y = yStart; y < yEnd; y++)
	{
		const int index = x + (y * frame.width);

		// Percent stepped from beginning to end on the column.
		const double yPercent =
			((static_cast<double>(y) + 0.50) - yProjStart) / (yProjEnd - yProjStart);

		// Vertical texture coordinate.
		const double v = vStart + ((vEnd - vStart) * yPercent);

		// Y position in texture.
		const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

		// Alpha is checked in this loop, and transparent texels are not drawn.
		const int textureIndex = textureX + (textureY * texture.width);
		const SkyTexel &texel = texture.texels[textureIndex];

		if (texel.a != 0.0)
		{
			// Get gradient color from sky gradient row cache.
			const Double3 &gradientColor = skyGradientRowCache.get(y);

			// If the gradient color behind the star is dark enough, then draw. Interpolate with a
			// range of intensities so stars don't immediately blink on/off when the gradient is a
			// certain color. Stars are generally small so I think it's okay to do more expensive
			// per-pixel operations here.
			constexpr double visThreshold = ShadingInfo::STAR_VIS_THRESHOLD; // Stars are becoming visible.
			constexpr double brightestThreshold = 32.0 / 255.0; // Stars are brightest.

			const double brightestComponent = std::max(
				std::max(gradientColor.x, gradientColor.y), gradientColor.z);
			const bool isDarkEnough = brightestComponent <= visThreshold;

			if (isDarkEnough)
			{
				const double gradientVisPercent = std::clamp(
					(brightestComponent - brightestThreshold) / (visThreshold - brightestThreshold),
					0.0, 1.0);

				// Texture color with shading.
				double colorR = texel.r;
				double colorG = texel.g;
				double colorB = texel.b;

				// Lerp with sky gradient for smoother transition between day and night.
				colorR += (gradientColor.x - colorR) * gradientVisPercent;
				colorG += (gradientColor.y - colorG) * gradientVisPercent;
				colorB += (gradientColor.z - colorB) * gradientVisPercent;

				// Clamp maximum (don't worry about negative values).
				const double high = 1.0;
				colorR = (colorR > high) ? high : colorR;
				colorG = (colorG > high) ? high : colorG;
				colorB = (colorB > high) ? high : colorB;

				// Convert floats to integers.
				const uint32_t colorRGB = static_cast<uint32_t>(
					((static_cast<uint8_t>(colorR * 255.0)) << 16) |
					((static_cast<uint8_t>(colorG * 255.0)) << 8) |
					((static_cast<uint8_t>(colorB * 255.0))));

				frame.colorBuffer[index] = colorRGB;
			}
		}
	}
}

void SoftwareRenderer::drawInitialVoxelSameFloor(int x, SNInt voxelX, int voxelY, WEInt voxelZ,
	const Camera &camera, const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint,
	const NewDouble2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
	const ShadingInfo &shadingInfo, int chunkDistance, double ceilingHeight, const LevelData &levelData,
	const BufferView<const VisibleLight> &visLights, const BufferView2D<const VisibleLightList> &visLightLists,
	const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups,
	OcclusionData &occlusion, const FrameView &frame)
{
	const auto &voxelGrid = levelData.getVoxelGrid();
	const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
	const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
	const double voxelHeight = ceilingHeight;
	const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
	const VisibleLightList &visLightList = SoftwareRenderer::getVisibleLightList(
		visLightLists, voxelX, voxelZ, absoluteEyeVoxel.x, absoluteEyeVoxel.z,
		voxelGrid.getWidth(), voxelGrid.getDepth(), chunkDistance);

	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		// Draw inner ceiling, wall, and floor.
		const VoxelDefinition::WallData &wallData = voxelDef.wall;

		const NewDouble3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelHeight,
			farPoint.y);
		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			farCeilingPoint.y,
			nearPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			voxelYReal,
			farPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			farFloorPoint.y,
			nearPoint.y);

		const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
			nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		// Ceiling.
		SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
			nearZ, farZ, -Double3::UnitY, textures.getTexture(wallData.ceilingTextureAssetRef),
			fadePercent, visLights, visLightList, shadingInfo, occlusion, frame);

		// Wall.
		const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(farPoint, visLights, visLightList);
		SoftwareRenderer::drawPixels(x, drawRanges.at(1), farZ, wallU, 0.0,
			Constants::JustBelowOne, wallNormal, textures.getTexture(wallData.sideTextureAssetRef),
			fadePercent, wallLightPercent, shadingInfo, occlusion, frame);

		// Floor.
		SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
			farZ, nearZ, Double3::UnitY, textures.getTexture(wallData.floorTextureAssetRef),
			fadePercent, visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
	{
		// Do nothing. Floors can only be seen from above.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
	{
		// Draw bottom of ceiling voxel if the camera is below it.
		if (absoluteEye.y < voxelYReal)
		{
			const VoxelDefinition::CeilingData &ceilingData = voxelDef.ceiling;

			const NewDouble3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.getTexture(ceilingData.textureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
	{
		const VoxelDefinition::RaisedData &raisedData = voxelDef.raised;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + (raisedData.yOffset * voxelHeight),
			nearPoint.y);

		// Draw order depends on the player's Y position relative to the platform.
		if (absoluteEye.y > nearCeilingPoint.y)
		{
			// Above platform.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				farCeilingPoint, nearCeilingPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else if (absoluteEye.y < nearFloorPoint.y)
		{
			// Below platform.
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else
		{
			// Between top and bottom.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
				nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint,
				camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(farPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), farZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal, textures.getTexture(raisedData.sideTextureAssetRef),
				wallLightPercent, shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
				farZ, nearZ, Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
	{
		const VoxelDefinition::DiagonalData &diagData = voxelDef.diagonal;

		// Find intersection.
		RayHit hit;
		const bool success = diagData.type1 ?
			SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
			SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

		if (success)
		{
			const NewDouble3 diagTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight,
				hit.point.y);
			const NewDouble3 diagBottomPoint(
				diagTopPoint.x,
				voxelYReal,
				diagTopPoint.z);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				diagTopPoint, diagBottomPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
				Constants::JustBelowOne, hit.normal, textures.getTexture(diagData.textureAssetRef), fadePercent,
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
	{
		// Do nothing. Transparent walls have no back-faces.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
	{
		const VoxelDefinition::EdgeData &edgeData = voxelDef.edge;

		// Find intersection.
		RayHit hit;
		const bool success = SoftwareRenderer::findInitialEdgeIntersection(
			voxelX, voxelZ, edgeData.facing, edgeData.flipped, nearPoint, farPoint,
			camera, ray, hit);

		if (success)
		{
			const NewDouble3 edgeTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight + edgeData.yOffset,
				hit.point.y);
			const NewDouble3 edgeBottomPoint(
				edgeTopPoint.x,
				voxelYReal + edgeData.yOffset,
				edgeTopPoint.z);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				edgeTopPoint, edgeBottomPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
				0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(edgeData.textureAssetRef),
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
	{
		// Render back-face.
		const VoxelDefinition::ChasmData &chasmData = voxelDef.chasm;

		const NewInt3 voxel(voxelX, 0, voxelZ);
		const VoxelInstance *chasmVoxelInst = levelData.tryGetVoxelInstance(voxel, VoxelInstance::Type::Chasm);
		const VoxelInstance::ChasmState *chasmState = (chasmVoxelInst != nullptr) ?
			&chasmVoxelInst->getChasmState() : nullptr;

		// Find which far face on the chasm was intersected.
		const NewDouble2 absoluteEye2D(absoluteEye.x, absoluteEye.z);
		const VoxelFacing2D farFacing = SoftwareRenderer::getInitialChasmFarFacing(voxelX, voxelZ, absoluteEye2D, ray);

		// Wet chasms and lava chasms are unaffected by ceiling height.
		const double chasmDepth = (chasmData.type == ArenaTypes::ChasmType::Dry) ?
			voxelHeight : ArenaVoxelUtils::WET_CHASM_DEPTH;

		const NewDouble3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelHeight,
			farPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			farCeilingPoint.y - chasmDepth,
			farPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			farFloorPoint.y,
			nearPoint.y);

		const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
			farCeilingPoint, farFloorPoint, nearFloorPoint, camera, frame);

		const ChasmTexture *chasmTexture;
		SoftwareRenderer::getChasmTextureGroupTexture(chasmTextureGroups, chasmData.type,
			shadingInfo.chasmAnimPercent, &chasmTexture);

		// Chasm floor (drawn before far wall for occlusion buffer).
		const Double3 floorNormal = Double3::UnitY;
		SoftwareRenderer::drawPerspectiveChasmPixels(x, drawRanges.at(1), farPoint, nearPoint,
			farZ, nearZ, floorNormal, RendererUtils::isChasmEmissive(chasmData.type),
			*chasmTexture, shadingInfo, occlusion, frame);

		// Far.
		if ((chasmState != nullptr) && chasmState->faceIsVisible(farFacing))
		{
			const double farU = [&farPoint, farFacing]()
			{
				const double uVal = [&farPoint, farFacing]()
				{
					if (farFacing == VoxelFacing2D::PositiveX)
					{
						return farPoint.y - std::floor(farPoint.y);
					}
					else if (farFacing == VoxelFacing2D::NegativeX)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else if (farFacing == VoxelFacing2D::PositiveZ)
					{
						return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
					}
					else
					{
						return farPoint.x - std::floor(farPoint.x);
					}
				}();

				return std::clamp(uVal, 0.0, Constants::JustBelowOne);
			}();

			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(farPoint, visLights, visLightList);

			const Double3 farNormal = -VoxelUtils::getNormal(farFacing);
			SoftwareRenderer::drawChasmPixels(x, drawRanges.at(0), farZ, farU, 0.0,
				Constants::JustBelowOne, farNormal, RendererUtils::isChasmEmissive(chasmData.type),
				textures.getTexture(chasmData.textureAssetRef), *chasmTexture, wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Door)
	{
		const VoxelDefinition::DoorData &doorData = voxelDef.door;
		const double percentOpen = RendererUtils::getDoorPercentOpen(voxelX, voxelZ, levelData);

		RayHit hit;
		const bool success = SoftwareRenderer::findInitialDoorIntersection(voxelX, voxelZ,
			doorData.type, percentOpen, nearPoint, farPoint, camera, ray, voxelGrid, hit);

		if (success)
		{
			if (doorData.type == ArenaTypes::DoorType::Swinging)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Sliding)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Raising)
			{
				// Top point is fixed, bottom point depends on percent open.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal + raisedAmount,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);

				// The start of the vertical texture coordinate depends on the percent open.
				const double vStart = raisedAmount / voxelHeight;

				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, vStart, Constants::JustBelowOne, hit.normal,
					textures.getTexture(doorData.textureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Splitting)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
		}
	}
}

void SoftwareRenderer::drawInitialVoxelAbove(int x, SNInt voxelX, int voxelY, WEInt voxelZ,
	const Camera &camera, const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint,
	const NewDouble2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
	const ShadingInfo &shadingInfo, int chunkDistance, double ceilingHeight, const LevelData &levelData,
	const BufferView<const VisibleLight> &visLights, const BufferView2D<const VisibleLightList> &visLightLists,
	const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups,
	OcclusionData &occlusion, const FrameView &frame)
{
	const auto &voxelGrid = levelData.getVoxelGrid();
	const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
	const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
	const double voxelHeight = ceilingHeight;
	const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
	const VisibleLightList &visLightList = SoftwareRenderer::getVisibleLightList(
		visLightLists, voxelX, voxelZ, absoluteEyeVoxel.x, absoluteEyeVoxel.z,
		voxelGrid.getWidth(), voxelGrid.getDepth(), chunkDistance);

	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		const VoxelDefinition::WallData &wallData = voxelDef.wall;

		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			nearFloorPoint.y,
			farPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			nearFloorPoint, farFloorPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		// Floor.
		SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
			farZ, -Double3::UnitY, textures.getTexture(wallData.floorTextureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
	{
		// Do nothing. Floors can only be seen from above.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
	{
		// Draw bottom of ceiling voxel.
		const VoxelDefinition::CeilingData &ceilingData = voxelDef.ceiling;

		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			nearFloorPoint.y,
			farPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			nearFloorPoint, farFloorPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
			farZ, -Double3::UnitY, textures.getTexture(ceilingData.textureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
	{
		const VoxelDefinition::RaisedData &raisedData = voxelDef.raised;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + (raisedData.yOffset * voxelHeight),
			nearPoint.y);

		// Draw order depends on the player's Y position relative to the platform.
		if (absoluteEye.y > nearCeilingPoint.y)
		{
			// Above platform.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				farCeilingPoint, nearCeilingPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else if (absoluteEye.y < nearFloorPoint.y)
		{
			// Below platform.
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else
		{
			// Between top and bottom.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
				nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint,
				camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(farPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), farZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
				farZ, nearZ, Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
	{
		const VoxelDefinition::DiagonalData &diagData = voxelDef.diagonal;

		// Find intersection.
		RayHit hit;
		const bool success = diagData.type1 ?
			SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
			SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

		if (success)
		{
			const NewDouble3 diagTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight,
				hit.point.y);
			const NewDouble3 diagBottomPoint(
				diagTopPoint.x,
				voxelYReal,
				diagTopPoint.z);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				diagTopPoint, diagBottomPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
				Constants::JustBelowOne, hit.normal, textures.getTexture(diagData.textureAssetRef), fadePercent,
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
	{
		// Do nothing. Transparent walls have no back-faces.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
	{
		const VoxelDefinition::EdgeData &edgeData = voxelDef.edge;

		// Find intersection.
		RayHit hit;
		const bool success = SoftwareRenderer::findInitialEdgeIntersection(
			voxelX, voxelZ, edgeData.facing, edgeData.flipped, nearPoint, farPoint,
			camera, ray, hit);

		if (success)
		{
			const NewDouble3 edgeTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight + edgeData.yOffset,
				hit.point.y);
			const NewDouble3 edgeBottomPoint(
				hit.point.x,
				voxelYReal + edgeData.yOffset,
				hit.point.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				edgeTopPoint, edgeBottomPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
				0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(edgeData.textureAssetRef),
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
	{
		// Ignore. Chasms should never be above the player's voxel.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Door)
	{
		const VoxelDefinition::DoorData &doorData = voxelDef.door;
		const double percentOpen = RendererUtils::getDoorPercentOpen(voxelX, voxelZ, levelData);

		RayHit hit;
		const bool success = SoftwareRenderer::findInitialDoorIntersection(voxelX, voxelZ,
			doorData.type, percentOpen, nearPoint, farPoint, camera, ray, voxelGrid, hit);

		if (success)
		{
			if (doorData.type == ArenaTypes::DoorType::Swinging)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Sliding)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Raising)
			{
				// Top point is fixed, bottom point depends on percent open.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal + raisedAmount,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);

				// The start of the vertical texture coordinate depends on the percent open.
				const double vStart = raisedAmount / voxelHeight;

				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, vStart, Constants::JustBelowOne, hit.normal,
					textures.getTexture(doorData.textureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Splitting)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
		}
	}
}

void SoftwareRenderer::drawInitialVoxelBelow(int x, SNInt voxelX, int voxelY, WEInt voxelZ,
	const Camera &camera, const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint,
	const NewDouble2 &farPoint, double nearZ, double farZ, double wallU, const Double3 &wallNormal,
	const ShadingInfo &shadingInfo, int chunkDistance, double ceilingHeight, const LevelData &levelData,
	const BufferView<const VisibleLight> &visLights, const BufferView2D<const VisibleLightList> &visLightLists,
	const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups,
	OcclusionData &occlusion, const FrameView &frame)
{
	const auto &voxelGrid = levelData.getVoxelGrid();
	const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
	const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
	const double voxelHeight = ceilingHeight;
	const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
	const VisibleLightList &visLightList = SoftwareRenderer::getVisibleLightList(
		visLightLists, voxelX, voxelZ, absoluteEyeVoxel.x, absoluteEyeVoxel.z,
		voxelGrid.getWidth(), voxelGrid.getDepth(), chunkDistance);

	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		const VoxelDefinition::WallData &wallData = voxelDef.wall;

		const NewDouble3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelHeight,
			farPoint.y);
		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			farCeilingPoint.y,
			nearPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			farCeilingPoint, nearCeilingPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		// Ceiling.
		SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
			nearZ, Double3::UnitY, textures.getTexture(wallData.ceilingTextureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
	{
		// Draw top of floor voxel.
		const VoxelDefinition::FloorData &floorData = voxelDef.floor;

		const NewDouble3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelHeight,
			farPoint.y);
		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			farCeilingPoint.y,
			nearPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			farCeilingPoint, nearCeilingPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		// Ceiling.
		SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
			nearZ, Double3::UnitY, textures.getTexture(floorData.textureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
	{
		// Do nothing. Ceilings can only be seen from below.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
	{
		const VoxelDefinition::RaisedData &raisedData = voxelDef.raised;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + (raisedData.yOffset * voxelHeight),
			nearPoint.y);

		// Draw order depends on the player's Y position relative to the platform.
		if (absoluteEye.y > nearCeilingPoint.y)
		{
			// Above platform.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				farCeilingPoint, nearCeilingPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
				nearZ, Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else if (absoluteEye.y < nearFloorPoint.y)
		{
			// Below platform.
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else
		{
			// Between top and bottom.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeThreePart(
				nearCeilingPoint, farCeilingPoint, farFloorPoint, nearFloorPoint,
				camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(farPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), farZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(2), farPoint, nearPoint,
				farZ, nearZ, Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
	{
		const VoxelDefinition::DiagonalData &diagData = voxelDef.diagonal;

		// Find intersection.
		RayHit hit;
		const bool success = diagData.type1 ?
			SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
			SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

		if (success)
		{
			const NewDouble3 diagTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight,
				hit.point.y);
			const NewDouble3 diagBottomPoint(
				diagTopPoint.x,
				voxelYReal,
				diagTopPoint.z);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				diagTopPoint, diagBottomPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
				Constants::JustBelowOne, hit.normal, textures.getTexture(diagData.textureAssetRef), fadePercent,
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
	{
		// Do nothing. Transparent walls have no back-faces.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
	{
		const VoxelDefinition::EdgeData &edgeData = voxelDef.edge;

		// Find intersection.
		RayHit hit;
		const bool success = SoftwareRenderer::findInitialEdgeIntersection(
			voxelX, voxelZ, edgeData.facing, edgeData.flipped, nearPoint, farPoint,
			camera, ray, hit);

		if (success)
		{
			const NewDouble3 edgeTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight + edgeData.yOffset,
				hit.point.y);
			const NewDouble3 edgeBottomPoint(
				hit.point.x,
				voxelYReal + edgeData.yOffset,
				hit.point.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				edgeTopPoint, edgeBottomPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
				0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(edgeData.textureAssetRef),
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
	{
		// Render back-face.
		const VoxelDefinition::ChasmData &chasmData = voxelDef.chasm;

		const NewInt3 voxel(voxelX, 0, voxelZ);
		const VoxelInstance *chasmVoxelInst = levelData.tryGetVoxelInstance(voxel, VoxelInstance::Type::Chasm);
		const VoxelInstance::ChasmState *chasmState = (chasmVoxelInst != nullptr) ?
			&chasmVoxelInst->getChasmState() : nullptr;

		// Find which far face on the chasm was intersected.
		const NewDouble2 absoluteEye2D(absoluteEye.x, absoluteEye.z);
		const VoxelFacing2D farFacing = SoftwareRenderer::getInitialChasmFarFacing(voxelX, voxelZ, absoluteEye2D, ray);

		// Wet chasms and lava chasms are unaffected by ceiling height.
		const double chasmDepth = (chasmData.type == ArenaTypes::ChasmType::Dry) ?
			voxelHeight : ArenaVoxelUtils::WET_CHASM_DEPTH;

		const NewDouble3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelHeight,
			farPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			farCeilingPoint.y - chasmDepth,
			farPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			farFloorPoint.y,
			nearPoint.y);

		const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
			farCeilingPoint, farFloorPoint, nearFloorPoint, camera, frame);

		const ChasmTexture *chasmTexture;
		SoftwareRenderer::getChasmTextureGroupTexture(chasmTextureGroups, chasmData.type,
			shadingInfo.chasmAnimPercent, &chasmTexture);

		// Chasm floor (drawn before far wall for occlusion buffer).
		const Double3 floorNormal = Double3::UnitY;
		SoftwareRenderer::drawPerspectiveChasmPixels(x, drawRanges.at(1), farPoint, nearPoint,
			farZ, nearZ, floorNormal, RendererUtils::isChasmEmissive(chasmData.type),
			*chasmTexture, shadingInfo, occlusion, frame);

		// Far.
		if ((chasmState != nullptr) && chasmState->faceIsVisible(farFacing))
		{
			const double farU = [&farPoint, farFacing]()
			{
				const double uVal = [&farPoint, farFacing]()
				{
					if (farFacing == VoxelFacing2D::PositiveX)
					{
						return farPoint.y - std::floor(farPoint.y);
					}
					else if (farFacing == VoxelFacing2D::NegativeX)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else if (farFacing == VoxelFacing2D::PositiveZ)
					{
						return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
					}
					else
					{
						return farPoint.x - std::floor(farPoint.x);
					}
				}();

				return std::clamp(uVal, 0.0, Constants::JustBelowOne);
			}();

			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(farPoint, visLights, visLightList);

			const Double3 farNormal = -VoxelUtils::getNormal(farFacing);
			SoftwareRenderer::drawChasmPixels(x, drawRanges.at(0), farZ, farU, 0.0,
				Constants::JustBelowOne, farNormal, RendererUtils::isChasmEmissive(chasmData.type),
				textures.getTexture(chasmData.textureAssetRef), *chasmTexture, wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Door)
	{
		const VoxelDefinition::DoorData &doorData = voxelDef.door;
		const double percentOpen = RendererUtils::getDoorPercentOpen(voxelX, voxelZ, levelData);

		RayHit hit;
		const bool success = SoftwareRenderer::findInitialDoorIntersection(voxelX, voxelZ,
			doorData.type, percentOpen, nearPoint, farPoint, camera, ray, voxelGrid, hit);

		if (success)
		{
			if (doorData.type == ArenaTypes::DoorType::Swinging)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Sliding)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Raising)
			{
				// Top point is fixed, bottom point depends on percent open.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal + raisedAmount,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);

				// The start of the vertical texture coordinate depends on the percent open.
				const double vStart = raisedAmount / voxelHeight;

				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, vStart, Constants::JustBelowOne, hit.normal,
					textures.getTexture(doorData.textureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Splitting)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
		}
	}
}

void SoftwareRenderer::drawInitialVoxelColumn(int x, SNInt voxelX, WEInt voxelZ, const Camera &camera,
	const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
	double nearZ, double farZ, const ShadingInfo &shadingInfo, int chunkDistance, double ceilingHeight,
	const LevelData &levelData, const BufferView<const VisibleLight> &visLights,
	const BufferView2D<const VisibleLightList> &visLightLists, const VoxelTextures &textures,
	const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame)
{
	// This method handles some special cases such as drawing the back-faces of wall sides.

	// When clamping Y values for drawing ranges, subtract 0.5 from starts and add 0.5 to 
	// ends before converting to integers because the drawing methods sample at the center 
	// of pixels. The clamping function depends on which side of the range is being clamped; 
	// either way, the drawing range should be contained within the projected range at the 
	// sub-pixel level. This ensures that the vertical texture coordinate is always within 0->1.

	const double wallU = [&farPoint, facing]()
	{
		const double uVal = [&farPoint, facing]()
		{
			if (facing == VoxelFacing2D::PositiveX)
			{
				return farPoint.y - std::floor(farPoint.y);
			}
			else if (facing == VoxelFacing2D::NegativeX)
			{
				return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
			}
			else if (facing == VoxelFacing2D::PositiveZ)
			{
				return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
			}
			else
			{
				return farPoint.x - std::floor(farPoint.x);
			}
		}();

		return std::clamp(uVal, 0.0, Constants::JustBelowOne);
	}();

	// Normal of the wall for the incoming ray, potentially shared between multiple voxels in
	// this voxel column.
	const Double3 wallNormal = -VoxelUtils::getNormal(facing);

	// Relative Y voxel coordinate of the camera, compensating for the ceiling height.
	const int adjustedVoxelY = camera.getAdjustedEyeVoxelY(ceilingHeight);

	// Draw the player's current voxel first.
	SoftwareRenderer::drawInitialVoxelSameFloor(x, voxelX, adjustedVoxelY, voxelZ, camera, ray, facing,
		nearPoint, farPoint, nearZ, farZ, wallU, wallNormal, shadingInfo, chunkDistance, ceilingHeight,
		levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);

	// Draw voxels below the player's voxel.
	for (int voxelY = (adjustedVoxelY - 1); voxelY >= 0; voxelY--)
	{
		SoftwareRenderer::drawInitialVoxelBelow(x, voxelX, voxelY, voxelZ, camera, ray, facing, nearPoint,
			farPoint, nearZ, farZ, wallU, wallNormal, shadingInfo, chunkDistance, ceilingHeight, levelData,
			visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
	}

	// Draw voxels above the player's voxel.
	const auto &voxelGrid = levelData.getVoxelGrid();
	for (int voxelY = (adjustedVoxelY + 1); voxelY < voxelGrid.getHeight(); voxelY++)
	{
		SoftwareRenderer::drawInitialVoxelAbove(x, voxelX, voxelY, voxelZ, camera, ray, facing, nearPoint,
			farPoint, nearZ, farZ, wallU, wallNormal, shadingInfo, chunkDistance, ceilingHeight, levelData,
			visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
	}
}

void SoftwareRenderer::drawVoxelSameFloor(int x, SNInt voxelX, int voxelY, WEInt voxelZ, const Camera &camera,
	const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, double nearZ,
	double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo, int chunkDistance,
	double ceilingHeight, const LevelData &levelData, const BufferView<const VisibleLight> &visLights,
	const BufferView2D<const VisibleLightList> &visLightLists, const VoxelTextures &textures,
	const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame)
{
	const auto &voxelGrid = levelData.getVoxelGrid();
	const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
	const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
	const double voxelHeight = ceilingHeight;
	const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;
	
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
	const VisibleLightList &visLightList = SoftwareRenderer::getVisibleLightList(
		visLightLists, voxelX, voxelZ, absoluteEyeVoxel.x, absoluteEyeVoxel.z,
		voxelGrid.getWidth(), voxelGrid.getDepth(), chunkDistance);

	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		// Draw side.
		const VoxelDefinition::WallData &wallData = voxelDef.wall;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelHeight,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			nearCeilingPoint, nearFloorPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);
		const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(nearPoint, visLights, visLightList);

		SoftwareRenderer::drawPixels(x, drawRange, nearZ, wallU, 0.0,
			Constants::JustBelowOne, wallNormal, textures.getTexture(wallData.sideTextureAssetRef), fadePercent,
			wallLightPercent, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
	{
		// Do nothing. Floors can only be seen from above.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
	{
		// Draw bottom of ceiling voxel if the camera is below it.
		if (absoluteEye.y < voxelYReal)
		{
			const VoxelDefinition::CeilingData &ceilingData = voxelDef.ceiling;

			const NewDouble3 nearFloorPoint(
				nearPoint.x,
				voxelYReal,
				nearPoint.y);
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
				farZ, -Double3::UnitY, textures.getTexture(ceilingData.textureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
	{
		const VoxelDefinition::RaisedData &raisedData = voxelDef.raised;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + (raisedData.yOffset * voxelHeight),
			nearPoint.y);

		// Draw order depends on the player's Y position relative to the platform.
		if (absoluteEye.y > nearCeilingPoint.y)
		{
			// Above platform.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint,
				farZ, nearZ, Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
		}
		else if (absoluteEye.y < nearFloorPoint.y)
		{
			// Below platform.
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(0), nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else
		{
			// Between top and bottom.
			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
	{
		const VoxelDefinition::DiagonalData &diagData = voxelDef.diagonal;

		// Find intersection.
		RayHit hit;
		const bool success = diagData.type1 ?
			SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
			SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

		if (success)
		{
			const NewDouble3 diagTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight,
				hit.point.y);
			const NewDouble3 diagBottomPoint(
				diagTopPoint.x,
				voxelYReal,
				diagTopPoint.z);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				diagTopPoint, diagBottomPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
				Constants::JustBelowOne, hit.normal, textures.getTexture(diagData.textureAssetRef), fadePercent,
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
	{
		// Draw transparent side.
		const VoxelDefinition::TransparentWallData &transparentWallData = voxelDef.transparentWall;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelHeight,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			nearCeilingPoint, nearFloorPoint, camera, frame);
		const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(nearPoint, visLights, visLightList);

		SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU, 0.0,
			Constants::JustBelowOne, wallNormal, textures.getTexture(transparentWallData.textureAssetRef),
			wallLightPercent, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
	{
		const VoxelDefinition::EdgeData &edgeData = voxelDef.edge;

		// Find intersection.
		RayHit hit;
		const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
			edgeData.facing, edgeData.flipped, facing, nearPoint, farPoint, wallU,
			camera, ray, hit);

		if (success)
		{
			const NewDouble3 edgeTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight + edgeData.yOffset,
				hit.point.y);
			const NewDouble3 edgeBottomPoint(
				hit.point.x,
				voxelYReal + edgeData.yOffset,
				hit.point.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				edgeTopPoint, edgeBottomPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
				0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(edgeData.textureAssetRef),
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
	{
		// Render front and back-faces.
		const VoxelDefinition::ChasmData &chasmData = voxelDef.chasm;

		const NewInt3 voxel(voxelX, 0, voxelZ);
		const VoxelInstance *chasmVoxelInst = levelData.tryGetVoxelInstance(voxel, VoxelInstance::Type::Chasm);
		const VoxelInstance::ChasmState *chasmState = (chasmVoxelInst != nullptr) ?
			&chasmVoxelInst->getChasmState() : nullptr;

		// Find which faces on the chasm were intersected.
		const VoxelFacing2D nearFacing = facing;
		const VoxelFacing2D farFacing = SoftwareRenderer::getChasmFarFacing(
			voxelX, voxelZ, nearFacing, camera, ray);

		// Wet chasms and lava chasms are unaffected by ceiling height.
		const double chasmDepth = (chasmData.type == ArenaTypes::ChasmType::Dry) ?
			voxelHeight : ArenaVoxelUtils::WET_CHASM_DEPTH;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelHeight,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			nearCeilingPoint.y - chasmDepth,
			nearPoint.y);
		const NewDouble3 farCeilingPoint(
			farPoint.x,
			nearCeilingPoint.y,
			farPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			nearFloorPoint.y,
			farPoint.y);

		const ChasmTexture *chasmTexture;
		SoftwareRenderer::getChasmTextureGroupTexture(chasmTextureGroups, chasmData.type,
			shadingInfo.chasmAnimPercent, &chasmTexture);

		// Near (drawn separately from far + chasm floor).
		if ((chasmState != nullptr) && chasmState->faceIsVisible(nearFacing))
		{
			const double nearU = Constants::JustBelowOne - wallU;
			const Double3 nearNormal = wallNormal;

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);

			SoftwareRenderer::drawChasmPixels(x, drawRange, nearZ, nearU, 0.0,
				Constants::JustBelowOne, nearNormal, RendererUtils::isChasmEmissive(chasmData.type),
				textures.getTexture(chasmData.textureAssetRef), *chasmTexture, wallLightPercent, shadingInfo, occlusion, frame);
		}

		const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
			farCeilingPoint, farFloorPoint, nearFloorPoint, camera, frame);

		// Chasm floor (drawn before far wall for occlusion buffer).
		const Double3 floorNormal = Double3::UnitY;
		SoftwareRenderer::drawPerspectiveChasmPixels(x, drawRanges.at(1), farPoint, nearPoint,
			farZ, nearZ, floorNormal, RendererUtils::isChasmEmissive(chasmData.type),
			*chasmTexture, shadingInfo, occlusion, frame);

		// Far.
		if ((chasmState != nullptr) && chasmState->faceIsVisible(farFacing))
		{
			const double farU = [&farPoint, farFacing]()
			{
				const double uVal = [&farPoint, farFacing]()
				{
					if (farFacing == VoxelFacing2D::PositiveX)
					{
						return farPoint.y - std::floor(farPoint.y);
					}
					else if (farFacing == VoxelFacing2D::NegativeX)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else if (farFacing == VoxelFacing2D::PositiveZ)
					{
						return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
					}
					else
					{
						return farPoint.x - std::floor(farPoint.x);
					}
				}();

				return std::clamp(uVal, 0.0, Constants::JustBelowOne);
			}();

			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(farPoint, visLights, visLightList);

			const Double3 farNormal = -VoxelUtils::getNormal(farFacing);
			SoftwareRenderer::drawChasmPixels(x, drawRanges.at(0), farZ, farU, 0.0,
				Constants::JustBelowOne, farNormal, RendererUtils::isChasmEmissive(chasmData.type),
				textures.getTexture(chasmData.textureAssetRef), *chasmTexture, wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Door)
	{
		const VoxelDefinition::DoorData &doorData = voxelDef.door;
		const double percentOpen = RendererUtils::getDoorPercentOpen(voxelX, voxelZ, levelData);

		RayHit hit;
		const bool success = SoftwareRenderer::findDoorIntersection(voxelX, voxelZ,
			doorData.type, percentOpen, facing, nearPoint, farPoint, wallU, hit);

		if (success)
		{
			if (doorData.type == ArenaTypes::DoorType::Swinging)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Sliding)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Raising)
			{
				// Top point is fixed, bottom point depends on percent open.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal + raisedAmount,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);

				// The start of the vertical texture coordinate depends on the percent open.
				const double vStart = raisedAmount / voxelHeight;

				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, vStart,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef), wallLightPercent,
					shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Splitting)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
		}
	}
}

void SoftwareRenderer::drawVoxelAbove(int x, SNInt voxelX, int voxelY, WEInt voxelZ, const Camera &camera,
	const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, double nearZ,
	double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo, int chunkDistance,
	double ceilingHeight, const LevelData &levelData, const BufferView<const VisibleLight> &visLights,
	const BufferView2D<const VisibleLightList> &visLightLists, const VoxelTextures &textures,
	const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame)
{
	const auto &voxelGrid = levelData.getVoxelGrid();
	const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
	const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
	const double voxelHeight = ceilingHeight;
	const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
	const VisibleLightList &visLightList = SoftwareRenderer::getVisibleLightList(
		visLightLists, voxelX, voxelZ, absoluteEyeVoxel.x, absoluteEyeVoxel.z,
		voxelGrid.getWidth(), voxelGrid.getDepth(), chunkDistance);

	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		const VoxelDefinition::WallData &wallData = voxelDef.wall;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelHeight,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			nearFloorPoint.y,
			farPoint.y);

		const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
			nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);
		const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(nearPoint, visLights, visLightList);

		// Wall.
		SoftwareRenderer::drawPixels(x, drawRanges.at(0), nearZ, wallU, 0.0,
			Constants::JustBelowOne, wallNormal, textures.getTexture(wallData.sideTextureAssetRef), fadePercent,
			wallLightPercent, shadingInfo, occlusion, frame);

		// Floor.
		SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
			nearZ, farZ, -Double3::UnitY, textures.getTexture(wallData.floorTextureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
	{
		// Do nothing. Floors can only be seen from above.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
	{
		// Draw bottom of ceiling voxel.
		const VoxelDefinition::CeilingData &ceilingData = voxelDef.ceiling;

		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			nearFloorPoint.y,
			farPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			nearFloorPoint, farFloorPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		SoftwareRenderer::drawPerspectivePixels(x, drawRange, nearPoint, farPoint, nearZ,
			farZ, -Double3::UnitY, textures.getTexture(ceilingData.textureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
	{
		const VoxelDefinition::RaisedData &raisedData = voxelDef.raised;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + (raisedData.yOffset * voxelHeight),
			nearPoint.y);

		// Draw order depends on the player's Y position relative to the platform.
		if (absoluteEye.y > nearCeilingPoint.y)
		{
			// Above platform.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint,
				farZ, nearZ, Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
		}
		else if (absoluteEye.y < nearFloorPoint.y)
		{
			// Below platform.
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(0), nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else
		{
			// Between top and bottom.
			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
	{
		const VoxelDefinition::DiagonalData &diagData = voxelDef.diagonal;

		// Find intersection.
		RayHit hit;
		const bool success = diagData.type1 ?
			SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
			SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

		if (success)
		{
			const NewDouble3 diagTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight,
				hit.point.y);
			const NewDouble3 diagBottomPoint(
				diagTopPoint.x,
				voxelYReal,
				diagTopPoint.z);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				diagTopPoint, diagBottomPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
				Constants::JustBelowOne, hit.normal, textures.getTexture(diagData.textureAssetRef), fadePercent,
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
	{
		// Draw transparent side.
		const VoxelDefinition::TransparentWallData &transparentWallData = voxelDef.transparentWall;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelHeight,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			nearCeilingPoint, nearFloorPoint, camera, frame);
		const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(nearPoint, visLights, visLightList);

		SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU, 0.0,
			Constants::JustBelowOne, wallNormal, textures.getTexture(transparentWallData.textureAssetRef),
			wallLightPercent, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
	{
		const VoxelDefinition::EdgeData &edgeData = voxelDef.edge;

		// Find intersection.
		RayHit hit;
		const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
			edgeData.facing, edgeData.flipped, facing, nearPoint, farPoint, wallU,
			camera, ray, hit);

		if (success)
		{
			const NewDouble3 edgeTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight + edgeData.yOffset,
				hit.point.y);
			const NewDouble3 edgeBottomPoint(
				hit.point.x,
				voxelYReal + edgeData.yOffset,
				hit.point.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				edgeTopPoint, edgeBottomPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
				0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(edgeData.textureAssetRef),
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
	{
		// Ignore. Chasms should never be above the player's voxel.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Door)
	{
		const VoxelDefinition::DoorData &doorData = voxelDef.door;
		const double percentOpen = RendererUtils::getDoorPercentOpen(voxelX, voxelZ, levelData);

		RayHit hit;
		const bool success = SoftwareRenderer::findDoorIntersection(voxelX, voxelZ,
			doorData.type, percentOpen, facing, nearPoint, farPoint, wallU, hit);

		if (success)
		{
			if (doorData.type == ArenaTypes::DoorType::Swinging)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Sliding)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Raising)
			{
				// Top point is fixed, bottom point depends on percent open.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal + raisedAmount,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);

				// The start of the vertical texture coordinate depends on the percent open.
				const double vStart = raisedAmount / voxelHeight;

				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, vStart,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef), wallLightPercent,
					shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Splitting)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
		}
	}
}

void SoftwareRenderer::drawVoxelBelow(int x, SNInt voxelX, int voxelY, WEInt voxelZ, const Camera &camera,
	const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint, double nearZ,
	double farZ, double wallU, const Double3 &wallNormal, const ShadingInfo &shadingInfo, int chunkDistance,
	double ceilingHeight, const LevelData &levelData, const BufferView<const VisibleLight> &visLights,
	const BufferView2D<const VisibleLightList> &visLightLists, const VoxelTextures &textures,
	const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame)
{
	const auto &voxelGrid = levelData.getVoxelGrid();
	const uint16_t voxelID = voxelGrid.getVoxel(voxelX, voxelY, voxelZ);
	const VoxelDefinition &voxelDef = voxelGrid.getVoxelDef(voxelID);
	const double voxelHeight = ceilingHeight;
	const double voxelYReal = static_cast<double>(voxelY) * voxelHeight;

	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
	const VisibleLightList &visLightList = SoftwareRenderer::getVisibleLightList(
		visLightLists, voxelX, voxelZ, absoluteEyeVoxel.x, absoluteEyeVoxel.z,
		voxelGrid.getWidth(), voxelGrid.getDepth(), chunkDistance);

	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		const VoxelDefinition::WallData &wallData = voxelDef.wall;

		const NewDouble3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelHeight,
			farPoint.y);
		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			farCeilingPoint.y,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);

		const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
			farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		// Ceiling.
		SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint, farZ,
			nearZ, Double3::UnitY, textures.getTexture(wallData.ceilingTextureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);

		// Wall.
		const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(nearPoint, visLights, visLightList);
		SoftwareRenderer::drawPixels(x, drawRanges.at(1), nearZ, wallU, 0.0,
			Constants::JustBelowOne, wallNormal, textures.getTexture(wallData.sideTextureAssetRef), fadePercent,
			wallLightPercent, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Floor)
	{
		// Draw top of floor voxel.
		const VoxelDefinition::FloorData &floorData = voxelDef.floor;

		const NewDouble3 farCeilingPoint(
			farPoint.x,
			voxelYReal + voxelHeight,
			farPoint.y);
		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			farCeilingPoint.y,
			nearPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			farCeilingPoint, nearCeilingPoint, camera, frame);
		const double fadePercent = RendererUtils::getFadingVoxelPercent(
			voxelX, voxelY, voxelZ, levelData);

		SoftwareRenderer::drawPerspectivePixels(x, drawRange, farPoint, nearPoint, farZ,
			nearZ, Double3::UnitY, textures.getTexture(floorData.textureAssetRef), fadePercent,
			visLights, visLightList, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Ceiling)
	{
		// Do nothing. Ceilings can only be seen from below.
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Raised)
	{
		const VoxelDefinition::RaisedData &raisedData = voxelDef.raised;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + ((raisedData.yOffset + raisedData.ySize) * voxelHeight),
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal + (raisedData.yOffset * voxelHeight),
			nearPoint.y);

		// Draw order depends on the player's Y position relative to the platform.
		if (absoluteEye.y > nearCeilingPoint.y)
		{
			// Above platform.
			const NewDouble3 farCeilingPoint(
				farPoint.x,
				nearCeilingPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				farCeilingPoint, nearCeilingPoint, nearFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Ceiling.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(0), farPoint, nearPoint,
				farZ, nearZ, Double3::UnitY, textures.getTexture(raisedData.ceilingTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(1), nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
		}
		else if (absoluteEye.y < nearFloorPoint.y)
		{
			// Below platform.
			const NewDouble3 farFloorPoint(
				farPoint.x,
				nearFloorPoint.y,
				farPoint.y);

			const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
				nearCeilingPoint, nearFloorPoint, farFloorPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);

			// Wall.
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);
			SoftwareRenderer::drawTransparentPixels(x, drawRanges.at(0), nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);

			// Floor.
			SoftwareRenderer::drawPerspectivePixels(x, drawRanges.at(1), nearPoint, farPoint,
				nearZ, farZ, -Double3::UnitY, textures.getTexture(raisedData.floorTextureAssetRef), fadePercent,
				visLights, visLightList, shadingInfo, occlusion, frame);
		}
		else
		{
			// Between top and bottom.
			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU,
				raisedData.vTop, raisedData.vBottom, wallNormal,
				textures.getTexture(raisedData.sideTextureAssetRef), wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Diagonal)
	{
		const VoxelDefinition::DiagonalData &diagData = voxelDef.diagonal;

		// Find intersection.
		RayHit hit;
		const bool success = diagData.type1 ?
			SoftwareRenderer::findDiag1Intersection(voxelX, voxelZ, nearPoint, farPoint, hit) :
			SoftwareRenderer::findDiag2Intersection(voxelX, voxelZ, nearPoint, farPoint, hit);

		if (success)
		{
			const NewDouble3 diagTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight,
				hit.point.y);
			const NewDouble3 diagBottomPoint(
				diagTopPoint.x,
				voxelYReal,
				diagTopPoint.z);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				diagTopPoint, diagBottomPoint, camera, frame);
			const double fadePercent = RendererUtils::getFadingVoxelPercent(
				voxelX, voxelY, voxelZ, levelData);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawPixels(x, drawRange, nearZ + hit.innerZ, hit.u, 0.0,
				Constants::JustBelowOne, hit.normal, textures.getTexture(diagData.textureAssetRef), fadePercent,
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::TransparentWall)
	{
		// Draw transparent side.
		const VoxelDefinition::TransparentWallData &transparentWallData = voxelDef.transparentWall;

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelHeight,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			voxelYReal,
			nearPoint.y);

		const auto drawRange = SoftwareRenderer::makeDrawRange(
			nearCeilingPoint, nearFloorPoint, camera, frame);
		const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(nearPoint, visLights, visLightList);

		SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, wallU, 0.0,
			Constants::JustBelowOne, wallNormal, textures.getTexture(transparentWallData.textureAssetRef),
			wallLightPercent, shadingInfo, occlusion, frame);
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Edge)
	{
		const VoxelDefinition::EdgeData &edgeData = voxelDef.edge;

		// Find intersection.
		RayHit hit;
		const bool success = SoftwareRenderer::findEdgeIntersection(voxelX, voxelZ,
			edgeData.facing, edgeData.flipped, facing, nearPoint, farPoint, wallU,
			camera, ray, hit);

		if (success)
		{
			const NewDouble3 edgeTopPoint(
				hit.point.x,
				voxelYReal + voxelHeight + edgeData.yOffset,
				hit.point.y);
			const NewDouble3 edgeBottomPoint(
				hit.point.x,
				voxelYReal + edgeData.yOffset,
				hit.point.y);

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				edgeTopPoint, edgeBottomPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(hit.point, visLights, visLightList);

			SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ, hit.u,
				0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(edgeData.textureAssetRef),
				wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Chasm)
	{
		// Render front and back-faces.
		const VoxelDefinition::ChasmData &chasmData = voxelDef.chasm;

		const NewInt3 voxel(voxelX, 0, voxelZ);
		const VoxelInstance *chasmVoxelInst = levelData.tryGetVoxelInstance(voxel, VoxelInstance::Type::Chasm);
		const VoxelInstance::ChasmState *chasmState = (chasmVoxelInst != nullptr) ?
			&chasmVoxelInst->getChasmState() : nullptr;

		// Wet chasms and lava chasms are unaffected by ceiling height.
		const double chasmDepth = (chasmData.type == ArenaTypes::ChasmType::Dry) ?
			voxelHeight : ArenaVoxelUtils::WET_CHASM_DEPTH;

		// Find which faces on the chasm were intersected.
		const VoxelFacing2D nearFacing = facing;
		const VoxelFacing2D farFacing = SoftwareRenderer::getChasmFarFacing(
			voxelX, voxelZ, nearFacing, camera, ray);

		const NewDouble3 nearCeilingPoint(
			nearPoint.x,
			voxelYReal + voxelHeight,
			nearPoint.y);
		const NewDouble3 nearFloorPoint(
			nearPoint.x,
			nearCeilingPoint.y - chasmDepth,
			nearPoint.y);
		const NewDouble3 farCeilingPoint(
			farPoint.x,
			nearCeilingPoint.y,
			farPoint.y);
		const NewDouble3 farFloorPoint(
			farPoint.x,
			nearFloorPoint.y,
			farPoint.y);

		const ChasmTexture *chasmTexture;
		SoftwareRenderer::getChasmTextureGroupTexture(chasmTextureGroups, chasmData.type,
			shadingInfo.chasmAnimPercent, &chasmTexture);

		// Near (drawn separately from far + chasm floor).
		if ((chasmState != nullptr) && chasmState->faceIsVisible(nearFacing))
		{
			const double nearU = Constants::JustBelowOne - wallU;
			const Double3 nearNormal = wallNormal;

			const auto drawRange = SoftwareRenderer::makeDrawRange(
				nearCeilingPoint, nearFloorPoint, camera, frame);
			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(nearPoint, visLights, visLightList);

			SoftwareRenderer::drawChasmPixels(x, drawRange, nearZ, nearU, 0.0,
				Constants::JustBelowOne, nearNormal, RendererUtils::isChasmEmissive(chasmData.type),
				textures.getTexture(chasmData.textureAssetRef), *chasmTexture, wallLightPercent, shadingInfo, occlusion, frame);
		}

		const auto drawRanges = SoftwareRenderer::makeDrawRangeTwoPart(
			farCeilingPoint, farFloorPoint, nearFloorPoint, camera, frame);

		// Chasm floor (drawn before far wall for occlusion buffer).
		const Double3 floorNormal = Double3::UnitY;
		SoftwareRenderer::drawPerspectiveChasmPixels(x, drawRanges.at(1), farPoint, nearPoint,
			farZ, nearZ, floorNormal, RendererUtils::isChasmEmissive(chasmData.type),
			*chasmTexture, shadingInfo, occlusion, frame);

		// Far.
		if ((chasmState != nullptr) && chasmState->faceIsVisible(farFacing))
		{
			const double farU = [&farPoint, farFacing]()
			{
				const double uVal = [&farPoint, farFacing]()
				{
					if (farFacing == VoxelFacing2D::PositiveX)
					{
						return farPoint.y - std::floor(farPoint.y);
					}
					else if (farFacing == VoxelFacing2D::NegativeX)
					{
						return Constants::JustBelowOne - (farPoint.y - std::floor(farPoint.y));
					}
					else if (farFacing == VoxelFacing2D::PositiveZ)
					{
						return Constants::JustBelowOne - (farPoint.x - std::floor(farPoint.x));
					}
					else
					{
						return farPoint.x - std::floor(farPoint.x);
					}
				}();

				return std::clamp(uVal, 0.0, Constants::JustBelowOne);
			}();

			const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
				LightContributionCap>(farPoint, visLights, visLightList);

			const Double3 farNormal = -VoxelUtils::getNormal(farFacing);
			SoftwareRenderer::drawChasmPixels(x, drawRanges.at(0), farZ, farU, 0.0,
				Constants::JustBelowOne, farNormal, RendererUtils::isChasmEmissive(chasmData.type),
				textures.getTexture(chasmData.textureAssetRef), *chasmTexture, wallLightPercent, shadingInfo, occlusion, frame);
		}
	}
	else if (voxelDef.type == ArenaTypes::VoxelType::Door)
	{
		const VoxelDefinition::DoorData &doorData = voxelDef.door;
		const double percentOpen = RendererUtils::getDoorPercentOpen(voxelX, voxelZ, levelData);

		RayHit hit;
		const bool success = SoftwareRenderer::findDoorIntersection(voxelX, voxelZ,
			doorData.type, percentOpen, facing, nearPoint, farPoint, wallU, hit);

		if (success)
		{
			if (doorData.type == ArenaTypes::DoorType::Swinging)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ + hit.innerZ,
					hit.u, 0.0, Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Sliding)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Raising)
			{
				// Top point is fixed, bottom point depends on percent open.
				const double minVisible = ArenaRenderUtils::DOOR_MIN_VISIBLE;
				const double raisedAmount = (voxelHeight * (1.0 - minVisible)) * percentOpen;

				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal + raisedAmount,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);

				// The start of the vertical texture coordinate depends on the percent open.
				const double vStart = raisedAmount / voxelHeight;

				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, vStart,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef), wallLightPercent,
					shadingInfo, occlusion, frame);
			}
			else if (doorData.type == ArenaTypes::DoorType::Splitting)
			{
				const NewDouble3 doorTopPoint(
					hit.point.x,
					voxelYReal + voxelHeight,
					hit.point.y);
				const NewDouble3 doorBottomPoint(
					doorTopPoint.x,
					voxelYReal,
					doorTopPoint.z);

				const auto drawRange = SoftwareRenderer::makeDrawRange(
					doorTopPoint, doorBottomPoint, camera, frame);
				const double wallLightPercent = SoftwareRenderer::getLightContributionAtPoint<
					LightContributionCap>(hit.point, visLights, visLightList);

				SoftwareRenderer::drawTransparentPixels(x, drawRange, nearZ, hit.u, 0.0,
					Constants::JustBelowOne, hit.normal, textures.getTexture(doorData.textureAssetRef),
					wallLightPercent, shadingInfo, occlusion, frame);
			}
		}
	}
}

void SoftwareRenderer::drawVoxelColumn(int x, SNInt voxelX, WEInt voxelZ, const Camera &camera,
	const Ray &ray, VoxelFacing2D facing, const NewDouble2 &nearPoint, const NewDouble2 &farPoint,
	double nearZ, double farZ, const ShadingInfo &shadingInfo, int chunkDistance, double ceilingHeight,
	const LevelData &levelData, const BufferView<const VisibleLight> &visLights,
	const BufferView2D<const VisibleLightList> &visLightLists, const VoxelTextures &textures,
	const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion, const FrameView &frame)
{
	// Much of the code here is duplicated from the initial voxel column drawing method, but
	// there are a couple differences, like the horizontal texture coordinate being flipped,
	// and the drawing orders being slightly modified. The reason for having so much code is
	// so we cover all the different ray casting cases efficiently. It would slow down this 
	// method if it had to worry about an "initialColumn" boolean that's always false in the
	// general case.

	// When clamping Y values for drawing ranges, subtract 0.5 from starts and add 0.5 to 
	// ends before converting to integers because the drawing methods sample at the center 
	// of pixels. The clamping function depends on which side of the range is being clamped; 
	// either way, the drawing range should be contained within the projected range at the 
	// sub-pixel level. This ensures that the vertical texture coordinate is always within 0->1.

	// Horizontal texture coordinate for the wall, potentially shared between multiple voxels
	// in this voxel column.
	const double wallU = [&nearPoint, facing]()
	{
		const double uVal = [&nearPoint, facing]()
		{
			if (facing == VoxelFacing2D::PositiveX)
			{
				return Constants::JustBelowOne - (nearPoint.y - std::floor(nearPoint.y));
			}
			else if (facing == VoxelFacing2D::NegativeX)
			{
				return nearPoint.y - std::floor(nearPoint.y);
			}
			else if (facing == VoxelFacing2D::PositiveZ)
			{
				return nearPoint.x - std::floor(nearPoint.x);
			}
			else
			{
				return Constants::JustBelowOne - (nearPoint.x - std::floor(nearPoint.x));
			}
		}();

		return std::clamp(uVal, 0.0, Constants::JustBelowOne);
	}();

	// Normal of the wall for the incoming ray, potentially shared between multiple voxels in
	// this voxel column.
	const Double3 wallNormal = VoxelUtils::getNormal(facing);

	// Relative Y voxel coordinate of the camera, compensating for the ceiling height.
	const int adjustedVoxelY = camera.getAdjustedEyeVoxelY(ceilingHeight);

	// Draw voxel straight ahead first.
	SoftwareRenderer::drawVoxelSameFloor(x, voxelX, adjustedVoxelY, voxelZ, camera, ray, facing,
		nearPoint, farPoint, nearZ, farZ, wallU, wallNormal, shadingInfo, chunkDistance, ceilingHeight,
		levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);

	// Draw voxels below the voxel.
	for (int voxelY = (adjustedVoxelY - 1); voxelY >= 0; voxelY--)
	{
		SoftwareRenderer::drawVoxelBelow(x, voxelX, voxelY, voxelZ, camera, ray, facing, nearPoint,
			farPoint, nearZ, farZ, wallU, wallNormal, shadingInfo, chunkDistance, ceilingHeight,
			levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
	}

	// Draw voxels above the voxel.
	const auto &voxelGrid = levelData.getVoxelGrid();
	for (int voxelY = (adjustedVoxelY + 1); voxelY < voxelGrid.getHeight(); voxelY++)
	{
		SoftwareRenderer::drawVoxelAbove(x, voxelX, voxelY, voxelZ, camera, ray, facing, nearPoint,
			farPoint, nearZ, farZ, wallU, wallNormal, shadingInfo, chunkDistance, ceilingHeight,
			levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
	}
}

void SoftwareRenderer::drawFlat(int startX, int endX, const VisibleFlat &flat, const Double3 &normal,
	const NewDouble2 &eye, const NewInt2 &eyeVoxelXZ, double horizonProjY, const ShadingInfo &shadingInfo,
	const Palette *overridePalette, int chunkDistance, const FlatTexture &texture,
	const BufferView<const VisibleLight> &visLights, const BufferView2D<const VisibleLightList> &visLightLists,
	int gridWidth, int gridDepth, const FrameView &frame)
{
	// X percents across the screen for the given start and end columns.
	const double startXPercent = (static_cast<double>(startX) + 0.50) / 
		static_cast<double>(frame.width);
	const double endXPercent = (static_cast<double>(endX) + 0.50) /
		static_cast<double>(frame.width);

	const bool startsInRange =
		(flat.startX >= startXPercent) && (flat.startX <= endXPercent);
	const bool endsInRange = 
		(flat.endX >= startXPercent) && (flat.endX <= endXPercent);
	const bool coversRange =
		(flat.startX <= startXPercent) && (flat.endX >= endXPercent);
	
	// Throw out the draw call if the flat is not in the X range.
	if (!startsInRange && !endsInRange && !coversRange)
	{
		return;
	}

	// Get the min and max X range of coordinates in screen-space. This range is completely 
	// contained within the flat.
	const double clampedStartXPercent = std::clamp(startXPercent, flat.startX, flat.endX);
	const double clampedEndXPercent = std::clamp(endXPercent, flat.startX, flat.endX);

	// The percentages from start to end within the flat.
	const double startFlatPercent = (clampedStartXPercent - flat.startX) /
		(flat.endX - flat.startX);
	const double endFlatPercent = (clampedEndXPercent - flat.startX) /
		(flat.endX - flat.startX);

	// Points interpolated between for per-column depth calculations in the XZ plane.
	const NewDouble3 startTopPoint = flat.topLeft.lerp(flat.topRight, startFlatPercent);
	const NewDouble3 endTopPoint = flat.topLeft.lerp(flat.topRight, endFlatPercent);

	// Horizontal texture coordinates in the flat. Although the flat percent can be
	// equal to 1.0, the texture coordinate needs to be less than 1.0.
	const double startU = std::clamp(startFlatPercent, 0.0, Constants::JustBelowOne);
	const double endU = std::clamp(endFlatPercent, 0.0, Constants::JustBelowOne);

	// Get the start and end coordinates of the projected points (Y values potentially
	// outside the screen).
	const double projectedXStart = clampedStartXPercent * frame.widthReal;
	const double projectedXEnd = clampedEndXPercent * frame.widthReal;
	const double projectedYStart = flat.startY * frame.heightReal;
	const double projectedYEnd = flat.endY * frame.heightReal;

	// Clamp the coordinates for where the flat starts and stops on the screen.
	const int xStart = RendererUtils::getLowerBoundedPixel(projectedXStart, frame.width);
	const int xEnd = RendererUtils::getUpperBoundedPixel(projectedXEnd, frame.width);
	const int yStart = RendererUtils::getLowerBoundedPixel(projectedYStart, frame.height);
	const int yEnd = RendererUtils::getUpperBoundedPixel(projectedYEnd, frame.height);

	// Shading on the texture.
	const Double3 shading(shadingInfo.ambient, shadingInfo.ambient, shadingInfo.ambient);

	// Use the override palette for citizen variations or the base palette for most entities.
	const Palette &palette = (overridePalette != nullptr) ? *overridePalette : shadingInfo.palette;

	// Draw by-column, similar to wall rendering.
	for (int x = xStart; x < xEnd; x++)
	{
		const double xPercent = ((static_cast<double>(x) + 0.50) - projectedXStart) /
			(projectedXEnd - projectedXStart);

		// Horizontal texture coordinate.
		const double u = startU + ((endU - startU) * xPercent);

		// Horizontal texel position.
		const int textureX = static_cast<int>(u * static_cast<double>(texture.width));

		const NewDouble3 topPoint = startTopPoint.lerp(endTopPoint, xPercent);

		// Get the true XZ distance for the depth.
		const NewDouble2 topPointXZ(topPoint.x, topPoint.z);
		const double depth = (topPointXZ - eye).length();

		// XZ coordinates that this vertical slice of the flat occupies.
		// @todo: should this be floor() + int() instead?
		const SNInt voxelX = static_cast<int>(topPointXZ.x);
		const WEInt voxelZ = static_cast<int>(topPointXZ.y);

		// Light contribution per column.
		const VisibleLightList &visLightList = SoftwareRenderer::getVisibleLightList(
			visLightLists, voxelX, voxelZ, eyeVoxelXZ.x, eyeVoxelXZ.y, gridWidth, gridDepth, chunkDistance);
		const double lightContributionPercent = SoftwareRenderer::getLightContributionAtPoint<
			LightContributionCap>(topPointXZ, visLights, visLightList);

		// Linearly interpolated fog.
		const Double3 &fogColor = shadingInfo.getFogColor();
		const double fogPercent = std::min(depth / shadingInfo.fogDistance, 1.0);

		for (int y = yStart; y < yEnd; y++)
		{
			const int index = x + (y * frame.width);

			if (depth <= frame.depthBuffer[index])
			{
				const double yPercent = ((static_cast<double>(y) + 0.50) - projectedYStart) /
					(projectedYEnd - projectedYStart);

				// Vertical texture coordinate.
				const double startV = 0.0;
				const double endV = Constants::JustBelowOne;
				const double v = startV + ((endV - startV) * yPercent);

				// Vertical texel position.
				const int textureY = static_cast<int>(v * static_cast<double>(texture.height));

				// Alpha is checked in this loop and transparent texels are not drawn.
				const int textureIndex = textureX + (textureY * texture.width);
				const FlatTexel &texel = texture.texels[textureIndex];
				const bool isTransparentTexel = texel.value == 0;

				if (!isTransparentTexel)
				{
					double colorR, colorG, colorB;
					if (ArenaRenderUtils::IsGhostTexel(texel.value))
					{
						// Ghost shader. The previously rendered pixel is diminished by some amount.
						const double alpha = static_cast<double>(texel.value) /
							static_cast<double>(ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_DIVISOR);

						const Double3 prevColor = Double3::fromRGB(frame.colorBuffer[index]);
						const double visPercent = std::clamp(1.0 - alpha, 0.0, 1.0);
						colorR = prevColor.x * visPercent;
						colorG = prevColor.y * visPercent;
						colorB = prevColor.z * visPercent;
					}
					else if (texture.reflective && ArenaRenderUtils::IsPuddleTexel(texel.value))
					{
						// Reflective texel (i.e. puddle).
						// Copy-paste the previously-drawn pixel from the Y pixel coordinate mirrored
						// around the horizon. If it is outside the screen, use the sky color instead.
						const int horizonY = static_cast<int>(horizonProjY * frame.heightReal);
						const int reflectedY = horizonY + (horizonY - y);
						const bool insideScreen = (reflectedY >= 0) && (reflectedY < frame.height);
						if (insideScreen)
						{
							// Read from mirrored position in frame buffer.
							const int reflectedIndex = x + (reflectedY * frame.width);
							const Double3 prevColor = Double3::fromRGB(frame.colorBuffer[reflectedIndex]);
							colorR = prevColor.x;
							colorG = prevColor.y;
							colorB = prevColor.z;
						}
						else
						{
							// Use sky color instead.
							const Double3 &skyColor = shadingInfo.skyColors.back();
							colorR = skyColor.x;
							colorG = skyColor.y;
							colorB = skyColor.z;
						}
					}
					else
					{
						// Texture color with shading and some conditional palette look-ups.
						const bool isRedSrc1 = (texel.value == ArenaRenderUtils::PALETTE_INDEX_RED_SRC1);
						const bool isRedSrc2 = (texel.value == ArenaRenderUtils::PALETTE_INDEX_RED_SRC2);
						const int paletteIndex = isRedSrc1 ? ArenaRenderUtils::PALETTE_INDEX_RED_DST1 :
							(isRedSrc2 ? ArenaRenderUtils::PALETTE_INDEX_RED_DST2 : texel.value);
						const Double4 texelColor = Double4::fromARGB(palette[paletteIndex].toARGB());

						const double shadingMax = 1.0;
						colorR = texelColor.x * std::min(shading.x + lightContributionPercent, shadingMax);
						colorG = texelColor.y * std::min(shading.y + lightContributionPercent, shadingMax);
						colorB = texelColor.z * std::min(shading.z + lightContributionPercent, shadingMax);
					}

					// Linearly interpolate with fog.
					colorR += (fogColor.x - colorR) * fogPercent;
					colorG += (fogColor.y - colorG) * fogPercent;
					colorB += (fogColor.z - colorB) * fogPercent;

					// Clamp maximum (don't worry about negative values).
					const double high = 1.0;
					colorR = (colorR > high) ? high : colorR;
					colorG = (colorG > high) ? high : colorG;
					colorB = (colorB > high) ? high : colorB;

					// Convert floats to integers.
					const uint32_t colorRGB = static_cast<uint32_t>(
						((static_cast<uint8_t>(colorR * 255.0)) << 16) |
						((static_cast<uint8_t>(colorG * 255.0)) << 8) |
						((static_cast<uint8_t>(colorB * 255.0))));

					frame.colorBuffer[index] = colorRGB;
					frame.depthBuffer[index] = depth;
				}
			}
		}
	}
}

template <bool NonNegativeDirX, bool NonNegativeDirZ>
void SoftwareRenderer::rayCast2DInternal(int x, const Camera &camera, const Ray &ray,
	const ShadingInfo &shadingInfo, int chunkDistance, double ceilingHeight, const LevelData &levelData,
	const BufferView<const VisibleLight> &visLights, const BufferView2D<const VisibleLightList> &visLightLists,
	const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion,
	const FrameView &frame)
{
	// Initially based on Lode Vandevenne's algorithm, this method of 2.5D ray casting is more 
	// expensive as it does not stop at the first wall intersection, and it also renders voxels 
	// above and below the current floor.

	// Some floating point behavior assumptions:
	// -> (value / 0.0) == infinity
	// -> (value / infinity) == 0.0
	// -> (int)(-0.8) == 0
	// -> (int)floor(-0.8) == -1
	// -> (int)ceil(-0.8) == 0

	constexpr SNInt stepX = NonNegativeDirX ? 1 : -1;
	constexpr WEInt stepZ = NonNegativeDirZ ? 1 : -1;
	constexpr SNDouble axisLenX = 1.0;
	constexpr WEDouble axisLenZ = 1.0;

	// Camera position values in absolute coordinate space.
	const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
	const NewDouble3 absoluteEyeVoxelReal = VoxelUtils::coordToNewPoint(camera.eyeVoxelReal);
	const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);

	// Delta distance is how far the ray has to go to step one voxel's worth along a certain axis.
	const SNDouble deltaDistX = (NonNegativeDirX ? axisLenX : -axisLenX) / ray.dirX;
	const WEDouble deltaDistZ = (NonNegativeDirZ ? axisLenZ : -axisLenZ) / ray.dirZ;

	// The initial delta distances are percentages of the delta distances, dependent on the ray
	// start position inside the voxel.
	const SNDouble initialDeltaDistPercentX = NonNegativeDirX ?
		(1.0 - ((absoluteEye.x - absoluteEyeVoxelReal.x) / axisLenX)) :
		((absoluteEye.x - absoluteEyeVoxelReal.x) / axisLenX);
	const WEDouble initialDeltaDistPercentZ = NonNegativeDirZ ?
		(1.0 - ((absoluteEye.z - absoluteEyeVoxelReal.z) / axisLenZ)) :
		((absoluteEye.z - absoluteEyeVoxelReal.z) / axisLenZ);

	// Initial delta distance is a fraction of delta distance based on the ray's position in
	// the initial voxel.
	const SNDouble initialDeltaDistX = deltaDistX * initialDeltaDistPercentX;
	const WEDouble initialDeltaDistZ = deltaDistZ * initialDeltaDistPercentZ;

	const VoxelGrid &voxelGrid = levelData.getVoxelGrid();
	const SNInt gridWidth = voxelGrid.getWidth();
	const int gridHeight = voxelGrid.getHeight();
	const WEInt gridDepth = voxelGrid.getDepth();

	// The Z distance from the camera to the wall, and the X or Z normal of the intersected
	// voxel face. The first Z distance is a special case, so it's brought outside the 
	// DDA loop.
	double zDistance;
	VoxelFacing2D facing;

	// Verify that the initial voxel coordinate is within the world bounds.
	bool voxelIsValid =
		(absoluteEyeVoxel.x >= 0) &&
		(absoluteEyeVoxel.y >= 0) &&
		(absoluteEyeVoxel.z >= 0) &&
		(absoluteEyeVoxel.x < gridWidth) &&
		(absoluteEyeVoxel.y < gridHeight) &&
		(absoluteEyeVoxel.z < gridDepth);

	if (voxelIsValid)
	{
		// Decide how far the wall is, and which voxel face was hit.
		if (initialDeltaDistX < initialDeltaDistZ)
		{
			zDistance = initialDeltaDistX;
			facing = NonNegativeDirX ? VoxelFacing2D::NegativeX : VoxelFacing2D::PositiveX;
		}
		else
		{
			zDistance = initialDeltaDistZ;
			facing = NonNegativeDirZ ? VoxelFacing2D::NegativeZ : VoxelFacing2D::PositiveZ;
		}

		// The initial near point is directly in front of the player in the near Z 
		// camera plane.
		const NewDouble2 initialNearPoint(
			absoluteEye.x + (ray.dirX * SoftwareRenderer::NEAR_PLANE),
			absoluteEye.z + (ray.dirZ * SoftwareRenderer::NEAR_PLANE));

		// The initial far point is the wall hit. This is used with the player's position 
		// for drawing the initial floor and ceiling.
		const NewDouble2 initialFarPoint(
			absoluteEye.x + (ray.dirX * zDistance),
			absoluteEye.z + (ray.dirZ * zDistance));

		// Draw all voxels in a column at the player's XZ coordinate.
		SoftwareRenderer::drawInitialVoxelColumn(x, absoluteEyeVoxel.x, absoluteEyeVoxel.z,
			camera, ray, facing, initialNearPoint, initialFarPoint, SoftwareRenderer::NEAR_PLANE,
			zDistance, shadingInfo, chunkDistance, ceilingHeight, levelData, visLights, visLightLists,
			textures, chasmTextureGroups, occlusion, frame);
	}

	// The current voxel coordinate in the DDA loop. For all intents and purposes,
	// the Y cell coordinate is constant.
	NewInt3 cell(absoluteEyeVoxel.x, absoluteEyeVoxel.y, absoluteEyeVoxel.z);
	
	// Delta distance sums in each component, starting at the initial wall hit. The lowest
	// component is the candidate for the next DDA loop.
	SNDouble deltaDistSumX = initialDeltaDistX;
	WEDouble deltaDistSumZ = initialDeltaDistZ;

	// Helper values for Z distance calculation per step.
	constexpr SNDouble halfOneMinusStepXReal = static_cast<double>((1 - stepX) / 2);
	constexpr WEDouble halfOneMinusStepZReal = static_cast<double>((1 - stepZ) / 2);

	// Lambda for stepping to the next XZ coordinate in the grid and updating the Z
	// distance for the current edge point.
	// @optimization: constexpr values in a lambda capture (stepX, zDistance values) are not baked in!!
	// - Only way to get the values baked in is 1) make template doDDAStep() method, or 2) no lambda.
	auto doDDAStep = [&ray, stepX, stepZ, &absoluteEye, deltaDistX, deltaDistZ, gridWidth, gridDepth,
		&zDistance, &facing, &voxelIsValid, &cell, &deltaDistSumX, &deltaDistSumZ,
		halfOneMinusStepXReal, halfOneMinusStepZReal]()
	{
		if (deltaDistSumX < deltaDistSumZ)
		{
			deltaDistSumX += deltaDistX;
			cell.x += stepX;
			facing = NonNegativeDirX ? VoxelFacing2D::NegativeX : VoxelFacing2D::PositiveX;
			voxelIsValid &= (cell.x >= 0) && (cell.x < gridWidth);
			zDistance = (((static_cast<double>(cell.x) - absoluteEye.x) + halfOneMinusStepXReal) / ray.dirX);
		}
		else
		{
			deltaDistSumZ += deltaDistZ;
			cell.z += stepZ;
			facing = NonNegativeDirZ ? VoxelFacing2D::NegativeZ : VoxelFacing2D::PositiveZ;
			voxelIsValid &= (cell.z >= 0) && (cell.z < gridDepth);
			zDistance = (((static_cast<double>(cell.z) - absoluteEye.z) + halfOneMinusStepZReal) / ray.dirZ);
		}
	};

	// Step forward in the grid once to leave the initial voxel and update the Z distance.
	doDDAStep();

	// Step through the voxel grid while the current coordinate is valid, the 
	// distance stepped is less than the distance at which fog is maximum, and
	// the column is not completely occluded.
	while (voxelIsValid && (zDistance < shadingInfo.fogDistance) &&
		(occlusion.yMin != occlusion.yMax))
	{
		// Store the cell coordinates, axis, and Z distance for wall rendering. The
		// loop needs to do another DDA step to calculate the far point.
		const SNInt savedCellX = cell.x;
		const WEInt savedCellZ = cell.z;
		const VoxelFacing2D savedFacing = facing;
		const double wallDistance = zDistance;

		// Decide which voxel in the XZ plane to step to next, and update the Z distance.
		doDDAStep();

		// Near and far points in the XZ plane. The near point is where the wall is, and 
		// the far point is used with the near point for drawing the floor and ceiling.
		const NewDouble2 nearPoint(
			absoluteEye.x + (ray.dirX * wallDistance),
			absoluteEye.z + (ray.dirZ * wallDistance));
		const NewDouble2 farPoint(
			absoluteEye.x + (ray.dirX * zDistance),
			absoluteEye.z + (ray.dirZ * zDistance));

		// Draw all voxels in a column at the given XZ coordinate.
		SoftwareRenderer::drawVoxelColumn(x, savedCellX, savedCellZ, camera, ray, savedFacing,
			nearPoint, farPoint, wallDistance, zDistance, shadingInfo, chunkDistance, ceilingHeight,
			levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
	}
}

void SoftwareRenderer::rayCast2D(int x, const Camera &camera, const Ray &ray, const ShadingInfo &shadingInfo,
	int chunkDistance, double ceilingHeight, const LevelData &levelData,
	const BufferView<const VisibleLight> &visLights, const BufferView2D<const VisibleLightList> &visLightLists,
	const VoxelTextures &textures, const ChasmTextureGroups &chasmTextureGroups, OcclusionData &occlusion,
	const FrameView &frame)
{
	// Certain values like the step delta are constant relative to the ray direction, allowing
	// for some compile-time constants and better code generation.
	const bool nonNegativeDirX = ray.dirX >= 0.0;
	const bool nonNegativeDirZ = ray.dirZ >= 0.0;

	if (nonNegativeDirX)
	{
		if (nonNegativeDirZ)
		{
			SoftwareRenderer::rayCast2DInternal<true, true>(x, camera, ray, shadingInfo, chunkDistance,
				ceilingHeight, levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
		}
		else
		{
			SoftwareRenderer::rayCast2DInternal<true, false>(x, camera, ray, shadingInfo, chunkDistance,
				ceilingHeight, levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
		}
	}
	else
	{
		if (nonNegativeDirZ)
		{
			SoftwareRenderer::rayCast2DInternal<false, true>(x, camera, ray, shadingInfo, chunkDistance,
				ceilingHeight, levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
		}
		else
		{
			SoftwareRenderer::rayCast2DInternal<false, false>(x, camera, ray, shadingInfo, chunkDistance,
				ceilingHeight, levelData, visLights, visLightLists, textures, chasmTextureGroups, occlusion, frame);
		}
	}
}

void SoftwareRenderer::drawSkyGradient(int startY, int endY, double gradientProjYTop,
	double gradientProjYBottom, Buffer<Double3> &skyGradientRowCache,
	std::atomic<bool> &shouldDrawStars, const ShadingInfo &shadingInfo, const FrameView &frame)
{
	// Lambda for drawing one row of colors and depth in the frame buffer.
	auto drawSkyRow = [&frame](int y, const Double3 &color)
	{
		uint32_t *colorPtr = frame.colorBuffer;
		double *depthPtr = frame.depthBuffer;
		const int startIndex = y * frame.width;
		const int endIndex = (y + 1) * frame.width;
		const uint32_t colorValue = color.toRGB();
		constexpr double depthValue = DEPTH_BUFFER_INFINITY;

		// Clear the color and depth of one row.
		for (int i = startIndex; i < endIndex; i++)
		{
			colorPtr[i] = colorValue;
			depthPtr[i] = depthValue;
		}
	};

	// While drawing the sky gradient, determine if it is dark enough for stars to be visible.
	bool isDarkEnough = false;

	for (int y = startY; y < endY; y++)
	{
		// Y percent across the screen.
		const double yPercent = (static_cast<double>(y) + 0.50) / frame.heightReal;

		// Y percent within the sky gradient.
		const double gradientPercent = SoftwareRenderer::getSkyGradientPercent(
			yPercent, gradientProjYTop, gradientProjYBottom);

		// Color of the sky gradient at the given percentage.
		const Double3 color = SoftwareRenderer::getSkyGradientRowColor(
			gradientPercent, shadingInfo);

		// Cache row color for star rendering.
		skyGradientRowCache.set(y, color);

		// Update star visibility.
		const double maxComp = std::max(std::max(color.x, color.y), color.z);
		isDarkEnough |= maxComp <= ShadingInfo::STAR_VIS_THRESHOLD;

		drawSkyRow(y, color);
	}

	if (isDarkEnough)
	{
		shouldDrawStars = true;
	}
}

void SoftwareRenderer::drawDistantSky(int startX, int endX, const VisDistantObjects &visDistantObjs,
	const std::vector<SkyTexture> &skyTextures, const Buffer<Double3> &skyGradientRowCache,
	bool shouldDrawStars, const ShadingInfo &shadingInfo, const FrameView &frame)
{
	enum class DistantRenderType { General, Moon, Star };

	// For each visible distant object, if it is at least partially within the start and end
	// X, then draw.
	auto drawDistantObj = [startX, endX, &skyTextures, &skyGradientRowCache, &shadingInfo, &frame](
		const VisDistantObject &obj, DistantRenderType renderType)
	{
		const SkyTexture &texture = *obj.texture;
		const DrawRange &drawRange = obj.drawRange;
		const double xProjStart = obj.xProjStart;
		const double xProjEnd = obj.xProjEnd;
		const int xDrawStart = std::max(obj.xStart, startX);
		const int xDrawEnd = std::min(obj.xEnd, endX);
		const bool emissive = obj.emissive;

		// Render the object based on its midpoint like the original game.
		for (int x = xDrawStart; x < xDrawEnd; x++)
		{
			// Percent X across the screen.
			const double xPercent = (static_cast<double>(x) + 0.50) / frame.widthReal;

			// Percentage across the horizontal span of the object in screen space.
			const double widthPercent = std::clamp(
				(xPercent - xProjStart) / (xProjEnd - xProjStart),
				0.0, Constants::JustBelowOne);

			// Horizontal texture coordinate.
			const double u = widthPercent;

			if (renderType == DistantRenderType::General)
			{
				SoftwareRenderer::drawDistantPixels(x, drawRange, u, 0.0,
					Constants::JustBelowOne, texture, emissive, shadingInfo, frame);
			}
			else if (renderType == DistantRenderType::Moon)
			{
				SoftwareRenderer::drawMoonPixels(x, drawRange, u, 0.0, Constants::JustBelowOne,
					texture, shadingInfo, frame);
			}
			else if (renderType == DistantRenderType::Star)
			{
				SoftwareRenderer::drawStarPixels(x, drawRange, u, 0.0, Constants::JustBelowOne,
					texture, skyGradientRowCache, shadingInfo, frame);
			}
		}
	};

	// Lambda for drawing a group of distant objects with some render type.
	auto drawDistantObjRange = [&visDistantObjs, &drawDistantObj](int start, int end,
		DistantRenderType renderType)
	{
		DebugAssert(start >= 0);
		DebugAssert(end <= visDistantObjs.objs.size());

		// Reverse iterate so objects are drawn far to near.
		const VisDistantObject *objs = visDistantObjs.objs.data();
		for (int i = end - 1; i >= start; i--)
		{
			drawDistantObj(objs[i], renderType);
		}
	};

	// Stars are only drawn when the sky gradient is dark enough. This saves on performance during
	// the daytime.
	if (shouldDrawStars)
	{
		drawDistantObjRange(visDistantObjs.starStart, visDistantObjs.starEnd, DistantRenderType::Star);
	}

	drawDistantObjRange(visDistantObjs.sunStart, visDistantObjs.sunEnd, DistantRenderType::General);
	drawDistantObjRange(visDistantObjs.moonStart, visDistantObjs.moonEnd, DistantRenderType::Moon);
	drawDistantObjRange(visDistantObjs.airStart, visDistantObjs.airEnd, DistantRenderType::General);
	drawDistantObjRange(visDistantObjs.animLandStart, visDistantObjs.animLandEnd, DistantRenderType::General);
	drawDistantObjRange(visDistantObjs.landStart, visDistantObjs.landEnd, DistantRenderType::General);
}

void SoftwareRenderer::drawVoxels(int startX, int stride, const Camera &camera, int chunkDistance,
	double ceilingHeight, const LevelData &levelData, const BufferView<const VisibleLight> &visLights,
	const BufferView2D<const VisibleLightList> &visLightLists, const VoxelTextures &voxelTextures,
	const ChasmTextureGroups &chasmTextureGroups, Buffer<OcclusionData> &occlusion,
	const ShadingInfo &shadingInfo, const FrameView &frame)
{
	const NewDouble2 forwardZoomed(camera.forwardZoomedX, camera.forwardZoomedZ);
	const NewDouble2 rightAspected(camera.rightAspectedX, camera.rightAspectedZ);

	// Draw pixel columns with spacing determined by the number of render threads.
	for (int x = startX; x < frame.width; x += stride)
	{
		// X percent across the screen.
		const double xPercent = (static_cast<double>(x) + 0.50) / frame.widthReal;

		// "Right" component of the ray direction, based on current screen X.
		const NewDouble2 rightComp = rightAspected * ((2.0 * xPercent) - 1.0);

		// Calculate the ray direction through the pixel.
		// - If un-normalized, it uses the Z distance, but the insides of voxels
		//   don't look right then.
		const NewDouble2 direction = (forwardZoomed + rightComp).normalized();
		const Ray ray(direction.x, direction.y);

		// Cast the 2D ray and fill in the column's pixels with color.
		SoftwareRenderer::rayCast2D(x, camera, ray, shadingInfo, chunkDistance, ceilingHeight, levelData,
			visLights, visLightLists, voxelTextures, chasmTextureGroups, occlusion.get(x), frame);
	}
}

void SoftwareRenderer::drawFlats(int startX, int endX, const Camera &camera,
	const Double3 &flatNormal, const std::vector<VisibleFlat> &visibleFlats,
	const FlatTextureGroups &flatTextureGroups, const ShadingInfo &shadingInfo, int chunkDistance,
	const BufferView<const VisibleLight> &visLights,
	const BufferView2D<const VisibleLightList> &visLightLists, SNInt gridWidth, WEInt gridDepth,
	const FrameView &frame)
{
	// Iterate through all flats, rendering those visible within the given X range of 
	// the screen.
	for (const VisibleFlat &flat : visibleFlats)
	{
		const NewDouble3 absoluteEye = VoxelUtils::coordToNewPoint(camera.eye);
		const NewInt3 absoluteEyeVoxel = VoxelUtils::coordToNewVoxel(camera.eyeVoxel);
		const NewDouble2 eye2D(absoluteEye.x, absoluteEye.z);
		const NewInt2 eyeVoxel2D(absoluteEyeVoxel.x, absoluteEyeVoxel.z);

		// Texture of the flat. It might be flipped horizontally as well, given by
		// the "flat.flipped" value.
		const EntityRenderID entityRenderID = flat.entityRenderID;
		const FlatTextureGroup &textureGroup = flatTextureGroups[entityRenderID];
		const FlatTexture &texture = textureGroup.getTexture(
			flat.animStateID, flat.animAngleID, flat.animTextureID);

		SoftwareRenderer::drawFlat(startX, endX, flat, flatNormal, eye2D, eyeVoxel2D, camera.horizonProjY,
			shadingInfo, flat.overridePalette, chunkDistance, texture, visLights, visLightLists, gridWidth,
			gridDepth, frame);
	}
}

void SoftwareRenderer::renderThreadLoop(RenderThreadData &threadData, int threadIndex, int startX,
	int endX, int startY, int endY)
{
	while (true)
	{
		// Initial wait condition. The lock must be unlocked after wait() so other threads can
		// lock it.
		std::unique_lock<std::mutex> lk(threadData.mutex);
		threadData.condVar.wait(lk, [&threadData]() { return threadData.go; });
		lk.unlock();

		// Received a go signal. Check if the renderer is being destroyed before doing anything.
		if (threadData.isDestructing)
		{
			break;
		}

		// Lambda for making a thread wait until others are finished rendering something. The last
		// thread to call this calls notify on all others.
		auto threadBarrier = [&threadData, &lk](auto &data)
		{
			lk.lock();
			data.threadsDone++;

			// If this was the last thread, notify all to continue.
			if (data.threadsDone == threadData.totalThreads)
			{
				lk.unlock();
				threadData.condVar.notify_all();
				return;
			}
			lk.unlock();
		};

		// Draw this thread's portion of the sky gradient.
		RenderThreadData::SkyGradient &skyGradient = threadData.skyGradient;
		SoftwareRenderer::drawSkyGradient(startY, endY, skyGradient.projectedYTop,
			skyGradient.projectedYBottom, *skyGradient.rowCache, skyGradient.shouldDrawStars,
			*threadData.shadingInfo, *threadData.frame);

		// Wait for other threads to finish the sky gradient.
		threadBarrier(skyGradient);

		// Wait for the visible distant object testing to finish.
		RenderThreadData::DistantSky &distantSky = threadData.distantSky;
		lk.lock();
		threadData.condVar.wait(lk, [&distantSky]() { return distantSky.doneVisTesting; });
		lk.unlock();

		// Draw this thread's portion of distant sky objects.
		SoftwareRenderer::drawDistantSky(startX, endX, *distantSky.visDistantObjs,
			*distantSky.skyTextures, *skyGradient.rowCache, skyGradient.shouldDrawStars,
			*threadData.shadingInfo, *threadData.frame);

		// Wait for other threads to finish distant sky objects.
		threadBarrier(distantSky);

		// Wait for visible light testing to finish.
		RenderThreadData::Voxels &voxels = threadData.voxels;
		lk.lock();
		threadData.condVar.wait(lk, [&voxels]() { return voxels.doneLightVisTesting; });
		lk.unlock();

		// Number of columns to skip per ray cast (for interleaved ray casting as a means of
		// load-balancing).
		const int strideX = threadData.totalThreads;

		// Draw this thread's portion of voxels.
		const BufferView<const VisibleLight> voxelsVisLightsView(voxels.visLights->data(),
			static_cast<int>(voxels.visLights->size()));
		const BufferView2D<const VisibleLightList> voxelsVisLightListsView(voxels.visLightLists->get(),
			voxels.visLightLists->getWidth(), voxels.visLightLists->getHeight());
		SoftwareRenderer::drawVoxels(threadIndex, strideX, *threadData.camera, voxels.chunkDistance,
			voxels.ceilingHeight, *voxels.levelData, voxelsVisLightsView, voxelsVisLightListsView,
			*voxels.voxelTextures, *voxels.chasmTextureGroups, *voxels.occlusion, *threadData.shadingInfo,
			*threadData.frame);

		// Wait for other threads to finish voxels.
		threadBarrier(voxels);

		// Wait for the visible flat sorting to finish.
		RenderThreadData::Flats &flats = threadData.flats;
		lk.lock();
		threadData.condVar.wait(lk, [&flats]() { return flats.doneSorting; });
		lk.unlock();

		// Draw this thread's portion of flats.
		const BufferView<const VisibleLight> flatsVisLightsView(flats.visLights->data(),
			static_cast<int>(flats.visLights->size()));
		const BufferView2D<const VisibleLightList> flatsVisLightListsView(flats.visLightLists->get(),
			flats.visLightLists->getWidth(), flats.visLightLists->getHeight());
		const VoxelGrid &voxelGrid = voxels.levelData->getVoxelGrid();
		SoftwareRenderer::drawFlats(startX, endX, *threadData.camera, *flats.flatNormal, *flats.visibleFlats,
			*flats.flatTextureGroups, *threadData.shadingInfo, voxels.chunkDistance, flatsVisLightsView,
			flatsVisLightListsView, voxelGrid.getWidth(), voxelGrid.getDepth(), *threadData.frame);

		// Wait for other threads to finish flats.
		threadBarrier(flats);
	}
}

void SoftwareRenderer::render(const CoordDouble3 &eye, const Double3 &direction, double fovY, double ambient,
	double daytimePercent, double chasmAnimPercent, double latitude, bool nightLightsAreActive, bool isExterior,
	bool playerHasLight, int chunkDistance, double ceilingHeight, const LevelData &levelData,
	const EntityDefinitionLibrary &entityDefLibrary, const Palette &palette, uint32_t *colorBuffer)
{
	// Constants for screen dimensions.
	const double widthReal = static_cast<double>(this->width);
	const double heightReal = static_cast<double>(this->height);
	const double aspect = widthReal / heightReal;

	// To account for tall pixels.
	const double projectionModifier = ArenaRenderUtils::TALL_PIXEL_RATIO;

	// 2.5D camera definition.
	const Camera camera(eye, direction, fovY, aspect, projectionModifier);

	// Normal of all flats (always facing the camera).
	const Double3 flatNormal = Double3(-camera.forwardX, 0.0, -camera.forwardZ).normalized();

	// Calculate shading information for this frame. Create some helper structs to keep similar
	// values together.
	const ShadingInfo shadingInfo(palette, this->skyPalette, daytimePercent, latitude, ambient,
		this->fogDistance, chasmAnimPercent, nightLightsAreActive, isExterior, playerHasLight);
	const FrameView frame(colorBuffer, this->depthBuffer.get(), this->width, this->height);

	// Projected Y range of the sky gradient.
	double gradientProjYTop, gradientProjYBottom;
	SoftwareRenderer::getSkyGradientProjectedYRange(camera, gradientProjYTop, gradientProjYBottom);

	// Set all the render-thread-specific shared data for this frame.
	this->threadData.init(this->renderThreads.getCount(), camera, shadingInfo, frame);
	this->threadData.skyGradient.init(gradientProjYTop, gradientProjYBottom, this->skyGradientRowCache);
	this->threadData.distantSky.init(this->visDistantObjs, this->skyTextures);
	this->threadData.voxels.init(chunkDistance, ceilingHeight, levelData, this->visibleLights,
		this->visLightLists, this->voxelTextures, this->chasmTextureGroups, this->occlusion);
	this->threadData.flats.init(flatNormal, this->visibleFlats, this->visibleLights, this->visLightLists,
		this->flatTextureGroups);

	// Give the render threads the go signal. They can work on the sky and voxels while this thread
	// does things like resetting occlusion and doing visible flat determination.
	// - Note about locks: they must always be locked before wait(), and stay locked after wait().
	std::unique_lock<std::mutex> lk(this->threadData.mutex);
	this->threadData.go = true;
	lk.unlock();
	this->threadData.condVar.notify_all();

	// Reset occlusion. Don't need to reset sky gradient row cache because it is written to before
	// it is read.
	this->occlusion.fill(OcclusionData(0, this->height));

	// Refresh the visible distant objects.
	this->updateVisibleDistantObjects(shadingInfo, camera, frame);

	lk.lock();
	this->threadData.condVar.wait(lk, [this]()
	{
		return this->threadData.skyGradient.threadsDone == this->threadData.totalThreads;
	});

	// Keep the render threads from getting the go signal again before the next frame.
	this->threadData.go = false;

	// Let the render threads know that they can start drawing distant objects.
	this->threadData.distantSky.doneVisTesting = true;
	lk.unlock();
	this->threadData.condVar.notify_all();

	// Refresh the visible flats. This should erase the old list, calculate a new list, and sort
	// it by depth.
	const VoxelGrid &voxelGrid = levelData.getVoxelGrid();
	const EntityManager &entityManager = levelData.getEntityManager();
	this->updateVisibleFlats(camera, shadingInfo, chunkDistance, ceilingHeight,
		voxelGrid, entityManager, entityDefLibrary);

	// Refresh visible light lists used for shading voxels and entities efficiently.
	this->updateVisibleLightLists(camera, chunkDistance, ceilingHeight, voxelGrid);

	lk.lock();
	this->threadData.condVar.wait(lk, [this]()
	{
		return this->threadData.distantSky.threadsDone == this->threadData.totalThreads;
	});

	// Let the render threads know that they can start drawing voxels.
	this->threadData.voxels.doneLightVisTesting = true;
	lk.unlock();
	this->threadData.condVar.notify_all();

	lk.lock();
	this->threadData.condVar.wait(lk, [this]()
	{
		return this->threadData.voxels.threadsDone == this->threadData.totalThreads;
	});

	// Let the render threads know that they can start drawing flats.
	this->threadData.flats.doneSorting = true;

	lk.unlock();
	this->threadData.condVar.notify_all();

	// Wait until render threads are done drawing flats.
	lk.lock();
	this->threadData.condVar.wait(lk, [this]()
	{
		return this->threadData.flats.threadsDone == this->threadData.totalThreads;
	});
}

void SoftwareRenderer::submitFrame(const RenderDefinitionGroup &defGroup, const RenderInstanceGroup &instGroup,
	const RenderCamera &camera, const RenderFrameSettings &settings)
{
	DebugNotImplemented();
}

void SoftwareRenderer::present()
{
	DebugNotImplemented();
}
