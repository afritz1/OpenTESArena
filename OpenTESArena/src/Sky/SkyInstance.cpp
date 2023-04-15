#include <cmath>

#include "ArenaSkyUtils.h"
#include "SkyAirDefinition.h"
#include "SkyDefinition.h"
#include "SkyInfoDefinition.h"
#include "SkyInstance.h"
#include "SkyLandDefinition.h"
#include "SkyMoonDefinition.h"
#include "SkyStarDefinition.h"
#include "SkySunDefinition.h"
#include "SkyUtils.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Assets/TextureManager.h"
#include "../Math/Random.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"
#include "../Weather/WeatherInstance.h"
#include "../World/MapDefinition.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

void SkyInstance::LoadedSkyObjectTextureEntry::init(const TextureAsset &textureAsset,
	ScopedObjectTextureRef &&objectTextureRef)
{
	this->textureAsset = textureAsset;
	this->objectTextureRef = std::move(objectTextureRef);
}

void SkyInstance::LoadedSmallStarTextureEntry::init(uint8_t paletteIndex, ScopedObjectTextureRef &&objectTextureRef)
{
	this->paletteIndex = paletteIndex;
	this->objectTextureRef = std::move(objectTextureRef);
}

SkyInstance::ObjectInstance::ObjectInstance()
{
	this->width = 0.0;
	this->height = 0.0;
	this->objectTextureID = -1;
	this->emissive = false;
}

void SkyInstance::ObjectInstance::init(const Double3 &baseDirection, double width, double height,
	ObjectTextureID objectTextureID, bool emissive)
{
	this->baseDirection = baseDirection;
	this->width = width;
	this->height = height;
	this->objectTextureID = objectTextureID;
	this->emissive = emissive;
}

const Double3 &SkyInstance::ObjectInstance::getBaseDirection() const
{
	return this->baseDirection;
}

const Double3 &SkyInstance::ObjectInstance::getTransformedDirection() const
{
	return this->transformedDirection;
}

double SkyInstance::ObjectInstance::getWidth() const
{
	return this->width;
}

double SkyInstance::ObjectInstance::getHeight() const
{
	return this->height;
}

ObjectTextureID SkyInstance::ObjectInstance::getObjectTextureID() const
{
	return this->objectTextureID;
}

bool SkyInstance::ObjectInstance::isEmissive() const
{
	return this->emissive;
}

void SkyInstance::ObjectInstance::setTransformedDirection(const Double3 &direction)
{
	this->transformedDirection = direction;
}

void SkyInstance::ObjectInstance::setDimensions(double width, double height)
{
	this->width = width;
	this->height = height;
}

void SkyInstance::ObjectInstance::setObjectTextureID(ObjectTextureID id)
{
	this->objectTextureID = id;
}

SkyInstance::AnimInstance::AnimInstance(int objectIndex, Buffer<ObjectTextureID> &&objectTextureIDs, double targetSeconds)
	: objectTextureIDs(std::move(objectTextureIDs))
{
	this->objectIndex = objectIndex;
	this->targetSeconds = targetSeconds;
	this->currentSeconds = 0.0;
}

SkyInstance::SkyInstance()
{
	this->landStart = -1;
	this->landEnd = -1;
	this->airStart = -1;
	this->airEnd = -1;
	this->moonStart = -1;
	this->moonEnd = -1;
	this->sunStart = -1;
	this->sunEnd = -1;
	this->starStart = -1;
	this->starEnd = -1;
	this->lightningStart = -1;
	this->lightningEnd = -1;
}

void SkyInstance::init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition,
	int currentDay, TextureManager &textureManager, Renderer &renderer)
{
	auto loadGeneralSkyObjectTexture = [this, &textureManager, &renderer](const TextureAsset &textureAsset)
	{
		const auto loadedIter = std::find_if(this->loadedSkyObjectTextures.begin(), this->loadedSkyObjectTextures.end(),
			[&textureAsset](const LoadedSkyObjectTextureEntry &loadedEntry)
		{
			return loadedEntry.textureAsset == textureAsset;
		});

		if (loadedIter != this->loadedSkyObjectTextures.end())
		{
			return;
		}

		const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(textureAsset);
		if (!textureBuilderID.has_value())
		{
			DebugLogWarning("Couldn't load sky object texture \"" + textureAsset.filename + "\".");
			return;
		}

		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);
		ObjectTextureID objectTextureID;
		if (!renderer.tryCreateObjectTexture(textureBuilder, &objectTextureID))
		{
			DebugLogWarning("Couldn't create sky object texture \"" + textureAsset.filename + "\".");
			return;
		}

		ScopedObjectTextureRef objectTextureRef(objectTextureID, renderer);
		LoadedSkyObjectTextureEntry newEntry;
		newEntry.init(textureAsset, std::move(objectTextureRef));
		this->loadedSkyObjectTextures.emplace_back(std::move(newEntry));
	};

	auto loadSmallStarTexture = [this, &textureManager, &renderer](uint8_t paletteIndex)
	{
		const auto loadedIter = std::find_if(this->loadedSmallStarTextures.begin(), this->loadedSmallStarTextures.end(),
			[paletteIndex](const LoadedSmallStarTextureEntry &loadedEntry)
		{
			return loadedEntry.paletteIndex == paletteIndex;
		});

		if (loadedIter != this->loadedSmallStarTextures.end())
		{
			return;
		}

		constexpr int textureWidth = 1;
		constexpr int textureHeight = textureWidth;

		ObjectTextureID objectTextureID;
		if (!renderer.tryCreateObjectTexture(textureWidth, textureHeight, 1, &objectTextureID))
		{
			DebugLogWarning("Couldn't create small star texture.");
			return;
		}

		LockedTexture lockedSmallStarTexture = renderer.lockObjectTexture(objectTextureID);
		if (!lockedSmallStarTexture.isValid())
		{
			DebugLogWarning("Couldn't lock small star texture for writing.");
			renderer.freeObjectTexture(objectTextureID);
			return;
		}

		DebugAssert(lockedSmallStarTexture.bytesPerTexel == 1);
		uint8_t *lockedSmallStarTexels = static_cast<uint8_t*>(lockedSmallStarTexture.texels);
		*lockedSmallStarTexels = paletteIndex;
		renderer.unlockObjectTexture(objectTextureID);

		ScopedObjectTextureRef objectTextureRef(objectTextureID, renderer);
		LoadedSmallStarTextureEntry newEntry;
		newEntry.init(paletteIndex, std::move(objectTextureRef));
		this->loadedSmallStarTextures.emplace_back(std::move(newEntry));
	};

	auto addGeneralObjectInst = [this, &textureManager, &renderer](const Double3 &baseDirection,
		const TextureAsset &textureAsset, bool emissive)
	{
		const auto cacheIter = std::find_if(this->loadedSkyObjectTextures.begin(), this->loadedSkyObjectTextures.end(),
			[&textureAsset](const LoadedSkyObjectTextureEntry &entry)
		{
			return entry.textureAsset == textureAsset;
		});

		if (cacheIter == this->loadedSkyObjectTextures.end())
		{
			DebugCrash("Expected sky object texture \"" + textureAsset.filename + "\" to be loaded.");
		}
		
		const ScopedObjectTextureRef &objectTextureRef = cacheIter->objectTextureRef;
		const ObjectTextureID objectTextureID = objectTextureRef.get();
		const int textureWidth = objectTextureRef.getWidth();
		const int textureHeight = objectTextureRef.getHeight();

		double width, height;
		SkyUtils::getSkyObjectDimensions(textureWidth, textureHeight, &width, &height);

		ObjectInstance objectInst;
		objectInst.init(baseDirection, width, height, objectTextureID, emissive);
		this->objectInsts.emplace_back(std::move(objectInst));
	};

	auto addSmallStarObjectInst = [this, &renderer](const Double3 &baseDirection, uint8_t paletteIndex)
	{
		const auto cacheIter = std::find_if(this->loadedSmallStarTextures.begin(), this->loadedSmallStarTextures.end(),
			[paletteIndex](const LoadedSmallStarTextureEntry &entry)
		{
			return entry.paletteIndex == paletteIndex;
		});

		if (cacheIter == this->loadedSmallStarTextures.end())
		{
			DebugCrash("Expected small star texture with color \"" + std::to_string(paletteIndex) + "\" to be loaded.");
		}

		const ScopedObjectTextureRef &objectTextureRef = cacheIter->objectTextureRef;
		const ObjectTextureID objectTextureID = objectTextureRef.get();
		const int textureWidth = objectTextureRef.getWidth();
		const int textureHeight = objectTextureRef.getHeight();

		double width, height;
		SkyUtils::getSkyObjectDimensions(textureWidth, textureHeight, &width, &height);

		constexpr bool emissive = true;
		ObjectInstance objectInst;
		objectInst.init(baseDirection, width, height, objectTextureID, emissive);
		this->objectInsts.emplace_back(std::move(objectInst));
	};

	auto addAnimInst = [this](int objectIndex, const Buffer<TextureAsset> &textureAssets, double targetSeconds)
	{
		// It is assumed that only general sky objects (not small stars) can have animations, and that their
		// textures have already been loaded earlier in SkyInstance::init().
		Buffer<ObjectTextureID> objectTextureIDs(textureAssets.getCount());
		for (int i = 0; i < textureAssets.getCount(); i++)
		{
			const TextureAsset &textureAsset = textureAssets.get(i);
			const auto cacheIter = std::find_if(this->loadedSkyObjectTextures.begin(), this->loadedSkyObjectTextures.end(),
				[&textureAsset](const LoadedSkyObjectTextureEntry &entry)
			{
				return entry.textureAsset == textureAsset;
			});

			DebugAssert(cacheIter != this->loadedSkyObjectTextures.end());
			const ObjectTextureID objectTextureID = cacheIter->objectTextureRef.get();
			objectTextureIDs.set(i, objectTextureID);
		}

		this->animInsts.emplace_back(objectIndex, std::move(objectTextureIDs), targetSeconds);
	};

	// Spawn all sky objects from the ready-to-bake format. Any animated objects start on their first frame.
	int landInstCount = 0;
	for (int i = 0; i < skyDefinition.getLandPlacementDefCount(); i++)
	{
		const SkyDefinition::LandPlacementDef &placementDef = skyDefinition.getLandPlacementDef(i);
		const SkyDefinition::LandDefID defID = placementDef.id;
		const SkyLandDefinition &skyLandDef = skyInfoDefinition.getLand(defID);

		// Load all textures for this land (mostly meant for volcanoes).
		for (int textureAssetIndex = 0; textureAssetIndex < skyLandDef.getTextureCount(); textureAssetIndex++)
		{
			const TextureAsset &textureAsset = skyLandDef.getTextureAsset(textureAssetIndex);
			loadGeneralSkyObjectTexture(textureAsset);
		}

		const TextureAsset &firstTextureAsset = skyLandDef.getTextureAsset(0);
		for (const Radians position : placementDef.positions)
		{
			// Convert radians to direction.
			const Radians angleY = 0.0;
			const Double3 direction = SkyUtils::getSkyObjectDirection(position, angleY);
			const bool emissive = skyLandDef.getShadingType() == SkyLandDefinition::ShadingType::Bright;
			addGeneralObjectInst(direction, firstTextureAsset, emissive);

			// Only land objects support animations (for now).
			if (skyLandDef.hasAnimation())
			{
				const int objectIndex = static_cast<int>(this->objectInsts.size()) - 1;
				Buffer<TextureAsset> textureAssets(skyLandDef.getTextureCount());
				for (int assetRefIndex = 0; assetRefIndex < skyLandDef.getTextureCount(); assetRefIndex++)
				{
					textureAssets.set(assetRefIndex, skyLandDef.getTextureAsset(assetRefIndex));
				}

				const double targetSeconds = static_cast<double>(skyLandDef.getTextureCount()) * ArenaSkyUtils::ANIMATED_LAND_SECONDS_PER_FRAME;
				addAnimInst(objectIndex, textureAssets, targetSeconds);
			}

			// Do position transform since it's only needed once at initialization for land objects.
			ObjectInstance &objectInst = this->objectInsts.back();
			objectInst.setTransformedDirection(objectInst.getBaseDirection());
		}

		landInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->landStart = 0;
	this->landEnd = this->landStart + landInstCount;

	int airInstCount = 0;
	for (int i = 0; i < skyDefinition.getAirPlacementDefCount(); i++)
	{
		const SkyDefinition::AirPlacementDef &placementDef = skyDefinition.getAirPlacementDef(i);
		const SkyDefinition::AirDefID defID = placementDef.id;
		const SkyAirDefinition &skyAirDef = skyInfoDefinition.getAir(defID);
		const TextureAsset &textureAsset = skyAirDef.getTextureAsset();
		loadGeneralSkyObjectTexture(textureAsset);

		for (const std::pair<Radians, Radians> &position : placementDef.positions)
		{
			// Convert X and Y radians to direction.
			const Radians angleX = position.first;
			const Radians angleY = position.second;
			const Double3 direction = SkyUtils::getSkyObjectDirection(angleX, angleY);
			constexpr bool emissive = false;
			addGeneralObjectInst(direction, textureAsset, emissive);

			// Do position transform since it's only needed once at initialization for air objects.
			ObjectInstance &objectInst = this->objectInsts.back();
			objectInst.setTransformedDirection(objectInst.getBaseDirection());
		}

		airInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->airStart = this->landEnd;
	this->airEnd = this->airStart + airInstCount;

	int moonInstCount = 0;
	for (int i = 0; i < skyDefinition.getMoonPlacementDefCount(); i++)
	{
		const SkyDefinition::MoonPlacementDef &placementDef = skyDefinition.getMoonPlacementDef(i);
		const SkyDefinition::MoonDefID defID = placementDef.id;
		const SkyMoonDefinition &skyMoonDef = skyInfoDefinition.getMoon(defID);

		// Get the image from the current day.
		DebugAssert(skyMoonDef.getTextureCount() > 0);
		const TextureAsset &textureAsset = skyMoonDef.getTextureAsset(currentDay);
		loadGeneralSkyObjectTexture(textureAsset);

		for (const SkyDefinition::MoonPlacementDef::Position &position : placementDef.positions)
		{
			// Default to the direction at midnight here, biased by the moon's bonus latitude and
			// orbit percent.
			// @todo: not sure this matches the original game but it looks fine.
			const Matrix4d moonLatitudeRotation = RendererUtils::getLatitudeRotation(position.bonusLatitude);
			const Matrix4d moonOrbitPercentRotation = Matrix4d::xRotation(position.orbitPercent * Constants::TwoPi);
			const Double3 baseDirection = -Double3::UnitY;
			Double4 direction4D(baseDirection.x, baseDirection.y, baseDirection.z, 0.0);
			direction4D = moonLatitudeRotation * direction4D;
			direction4D = moonOrbitPercentRotation * direction4D;

			constexpr bool emissive = true;
			addGeneralObjectInst(Double3(direction4D.x, direction4D.y, direction4D.z), textureAsset, emissive);
		}

		moonInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->moonStart = this->airEnd;
	this->moonEnd = this->moonStart + moonInstCount;

	int sunInstCount = 0;
	for (int i = 0; i < skyDefinition.getSunPlacementDefCount(); i++)
	{
		const SkyDefinition::SunPlacementDef &placementDef = skyDefinition.getSunPlacementDef(i);
		const SkyDefinition::SunDefID defID = placementDef.id;
		const SkySunDefinition &skySunDef = skyInfoDefinition.getSun(defID);
		const TextureAsset &textureAsset = skySunDef.getTextureAsset();
		loadGeneralSkyObjectTexture(textureAsset);

		for (const double position : placementDef.positions)
		{
			// Default to the direction at midnight here, biased by the sun's bonus latitude.
			const Matrix4d sunLatitudeRotation = RendererUtils::getLatitudeRotation(position);
			const Double3 baseDirection = -Double3::UnitY;
			const Double4 direction4D = sunLatitudeRotation *
				Double4(baseDirection.x, baseDirection.y, baseDirection.z, 0.0);
			constexpr bool emissive = true;
			addGeneralObjectInst(Double3(direction4D.x, direction4D.y, direction4D.z), textureAsset, emissive);
		}

		sunInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->sunStart = this->moonEnd;
	this->sunEnd = this->sunStart + sunInstCount;

	int starInstCount = 0;
	for (int i = 0; i < skyDefinition.getStarPlacementDefCount(); i++)
	{
		const SkyDefinition::StarPlacementDef &placementDef = skyDefinition.getStarPlacementDef(i);
		const SkyDefinition::StarDefID defID = placementDef.id;
		const SkyStarDefinition &skyStarDef = skyInfoDefinition.getStar(defID);

		// @todo: this is where the texture-id-from-texture-manager design is breaking, and getting
		// a renderer texture handle would be better. SkyInstance::init() should be able to allocate
		// textures IDs from the renderer eventually, and look up cached ones by string.
		const SkyStarDefinition::Type starType = skyStarDef.getType();
		if (starType == SkyStarDefinition::Type::Small)
		{
			// Small stars are 1x1 pixels.
			const SkyStarDefinition::SmallStar &smallStar = skyStarDef.getSmallStar();
			const uint8_t paletteIndex = smallStar.paletteIndex;
			loadSmallStarTexture(paletteIndex);

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				addSmallStarObjectInst(position, paletteIndex);
			}
		}
		else if (starType == SkyStarDefinition::Type::Large)
		{
			const SkyStarDefinition::LargeStar &largeStar = skyStarDef.getLargeStar();
			const TextureAsset &textureAsset = largeStar.textureAsset;
			loadGeneralSkyObjectTexture(textureAsset);

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				constexpr bool emissive = true;
				addGeneralObjectInst(position, textureAsset, emissive);
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(starType)));
		}

		starInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->starStart = this->sunEnd;
	this->starEnd = this->starStart + starInstCount;

	// Populate lightning bolt assets for random selection.
	const int lightningBoltDefCount = skyInfoDefinition.getLightningCount();
	if (lightningBoltDefCount > 0)
	{
		this->lightningAnimIndices.init(lightningBoltDefCount);

		for (int i = 0; i < lightningBoltDefCount; i++)
		{
			const SkyLightningDefinition &skyLightningDef = skyInfoDefinition.getLightning(i);

			// Load all textures for this lightning bolt.
			for (int textureAssetIndex = 0; textureAssetIndex < skyLightningDef.getTextureCount(); textureAssetIndex++)
			{
				const TextureAsset &textureAsset = skyLightningDef.getTextureAsset(textureAssetIndex);
				loadGeneralSkyObjectTexture(textureAsset);
			}

			const TextureAsset &firstTextureAsset = skyLightningDef.getTextureAsset(0);
			addGeneralObjectInst(Double3::Zero, firstTextureAsset, true);
			
			Buffer<TextureAsset> textureAssets(skyLightningDef.getTextureCount());
			for (int assetRefIndex = 0; assetRefIndex < skyLightningDef.getTextureCount(); assetRefIndex++)
			{
				textureAssets.set(assetRefIndex, skyLightningDef.getTextureAsset(assetRefIndex));
			}

			addAnimInst(static_cast<int>(this->objectInsts.size()) - 1, textureAssets, skyLightningDef.getAnimationSeconds());

			this->lightningAnimIndices.set(i, static_cast<int>(this->animInsts.size()) - 1);
		}

		this->lightningStart = this->starEnd;
		this->lightningEnd = this->lightningStart + lightningBoltDefCount;
	}
}

int SkyInstance::getLandStartIndex() const
{
	return this->landStart;
}

int SkyInstance::getLandEndIndex() const
{
	return this->landEnd;
}

int SkyInstance::getAirStartIndex() const
{
	return this->airStart;
}

int SkyInstance::getAirEndIndex() const
{
	return this->airEnd;
}

int SkyInstance::getStarStartIndex() const
{
	return this->starStart;
}

int SkyInstance::getStarEndIndex() const
{
	return this->starEnd;
}

int SkyInstance::getSunStartIndex() const
{
	return this->sunStart;
}

int SkyInstance::getSunEndIndex() const
{
	return this->sunEnd;
}

int SkyInstance::getMoonStartIndex() const
{
	return this->moonStart;
}

int SkyInstance::getMoonEndIndex() const
{
	return this->moonEnd;
}

int SkyInstance::getLightningStartIndex() const
{
	return this->lightningStart;
}

int SkyInstance::getLightningEndIndex() const
{
	return this->lightningEnd;
}

bool SkyInstance::isLightningVisible(int objectIndex) const
{
	return this->currentLightningBoltObjectIndex == objectIndex;
}

void SkyInstance::getSkyObject(int index, Double3 *outDirection, ObjectTextureID *outObjectTextureID,
	bool *outEmissive, double *outWidth, double *outHeight) const
{
	DebugAssertIndex(this->objectInsts, index);
	const ObjectInstance &objectInst = this->objectInsts[index];
	*outDirection = objectInst.getTransformedDirection();
	*outObjectTextureID = objectInst.getObjectTextureID();
	*outEmissive = objectInst.isEmissive();
	*outWidth = objectInst.getWidth();
	*outHeight = objectInst.getHeight();
}

std::optional<double> SkyInstance::tryGetObjectAnimPercent(int index) const
{
	// See if the object has an animation.
	for (int i = 0; i < static_cast<int>(this->animInsts.size()); i++)
	{
		const SkyInstance::AnimInstance &animInst = this->animInsts[i];
		if (animInst.objectIndex == index)
		{
			return animInst.currentSeconds / animInst.targetSeconds;
		}
	}

	// No animation for the object.
	return std::nullopt;
}

ObjectTextureID SkyInstance::getSkyColorsTextureID() const
{
	return this->skyColorsTextureRef.get();
}

bool SkyInstance::trySetActive(const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	TextureManager &textureManager, Renderer &renderer)
{
	// Although the sky could be treated the same way as visible entities (regenerated every frame), keep it this
	// way because the sky is not added to and removed from like entities are. The sky is baked once per level
	// and that's it.
	
	// @todo: tell RenderChunkManager to clear its sky geometry
	//DebugNotImplementedMsg("trySetActive");
	//renderer.clearSky();

	const MapType mapType = mapDefinition.getSubDefinition().type;
	const SkyDefinition &skyDefinition = [&activeLevelIndex, &mapDefinition, mapType]() -> const SkyDefinition&
	{
		const int skyIndex = [&activeLevelIndex, &mapDefinition, mapType]()
		{
			if ((mapType == MapType::Interior) || (mapType == MapType::City))
			{
				DebugAssert(activeLevelIndex.has_value());
				return mapDefinition.getSkyIndexForLevel(*activeLevelIndex);
			}
			else if (mapType == MapType::Wilderness)
			{
				return mapDefinition.getSkyIndexForLevel(0);
			}
			else
			{
				DebugUnhandledReturnMsg(int, std::to_string(static_cast<int>(mapType)));
			}
		}();

		return mapDefinition.getSky(skyIndex);
	}();

	// Set sky gradient colors.
	Buffer<uint32_t> skyColors(skyDefinition.getSkyColorCount());
	for (int i = 0; i < skyColors.getCount(); i++)
	{
		const Color &color = skyDefinition.getSkyColor(i);
		skyColors.set(i, color.toARGB());
	}

	ObjectTextureID skyColorsTextureID;
	if (!renderer.tryCreateObjectTexture(skyColors.getCount(), 1, 1, &skyColorsTextureID))
	{
		DebugLogError("Couldn't create sky colors texture.");
		return false;
	}

	this->skyColorsTextureRef = ScopedObjectTextureRef(skyColorsTextureID, renderer);
	LockedTexture lockedSkyColorsTexture = renderer.lockObjectTexture(this->skyColorsTextureRef.get());
	DebugAssert(lockedSkyColorsTexture.bytesPerTexel == 1);
	uint8_t *lockedSkyColorsTexels = static_cast<uint8_t*>(lockedSkyColorsTexture.texels);
	// @todo: change SkyDefinition sky colors to be 8-bit, or a TextureBuilder maybe in case of modding
	std::fill(lockedSkyColorsTexels, lockedSkyColorsTexels + skyColors.getCount(), 0);
	renderer.unlockObjectTexture(this->skyColorsTextureRef.get());

	// Set sky objects in the renderer.
	// @todo: I think we're going to be leaking/making duplicates of textures for every new sky instance we create
	// unless the sky instances get deleted automatically when MapInstance goes out of scope. Need to investigate.
	//renderer.setSky(*this, palette, textureManager); // @todo: this should set all the distant sky intermediate values in the render chunk manager equivalent for the sky I think.

	return true;
}

void SkyInstance::update(double dt, double latitude, double daytimePercent, const WeatherInstance &weatherInst,
	Random &random, const TextureManager &textureManager)
{
	// Update lightning (if any).
	if (weatherInst.hasRain())
	{
		const WeatherInstance::RainInstance &rainInst = weatherInst.getRain();
		const std::optional<WeatherInstance::RainInstance::Thunderstorm> &thunderstorm = rainInst.thunderstorm;
		if (thunderstorm.has_value() && thunderstorm->active)
		{
			DebugAssert(this->lightningAnimIndices.getCount() > 0);

			const std::optional<double> lightningBoltPercent = thunderstorm->getLightningBoltPercent();
			const bool visibilityChanged = this->currentLightningBoltObjectIndex.has_value() != lightningBoltPercent.has_value();
			if (visibilityChanged && lightningBoltPercent.has_value())
			{
				const int lightningGroupCount = this->lightningEnd - this->lightningStart;
				this->currentLightningBoltObjectIndex = this->lightningStart + random.next(lightningGroupCount);

				// Pick a new spot for the chosen lightning bolt.
				const Radians lightningAngleX = thunderstorm->lightningBoltAngle;
				const Double3 lightningDirection = SkyUtils::getSkyObjectDirection(lightningAngleX, 0.0);

				ObjectInstance &lightningObjInst = this->objectInsts[*this->currentLightningBoltObjectIndex];
				lightningObjInst.setTransformedDirection(lightningDirection);
			}

			if (lightningBoltPercent.has_value())
			{
				const int animInstIndex = this->lightningAnimIndices.get(*this->currentLightningBoltObjectIndex - this->lightningStart);
				AnimInstance &lightningAnimInst = this->animInsts[animInstIndex];
				lightningAnimInst.currentSeconds = *lightningBoltPercent * lightningAnimInst.targetSeconds;
			}
			else
			{
				this->currentLightningBoltObjectIndex = std::nullopt;
			}
		}
		else
		{
			this->currentLightningBoltObjectIndex = std::nullopt;
		}
	}

	// Update animations.
	const int animInstCount = static_cast<int>(this->animInsts.size());
	for (int i = 0; i < animInstCount; i++)
	{
		AnimInstance &animInst = this->animInsts[i];

		// Don't update if it's an inactive lightning bolt.
		const bool isLightningAnim = [this, i]()
		{
			if (this->lightningAnimIndices.getCount() == 0)
			{
				return false;
			}

			const auto iter = std::find(this->lightningAnimIndices.begin(), this->lightningAnimIndices.end(), i);
			return iter != this->lightningAnimIndices.end();
		}();

		if (isLightningAnim && !this->isLightningVisible(animInst.objectIndex))
		{
			continue;
		}

		animInst.currentSeconds += dt;
		if (animInst.currentSeconds >= animInst.targetSeconds)
		{
			animInst.currentSeconds = std::fmod(animInst.currentSeconds, animInst.targetSeconds);
		}

		const int imageCount = animInst.objectTextureIDs.getCount();
		const double animPercent = animInst.currentSeconds / animInst.targetSeconds;
		const int animIndex = std::clamp(static_cast<int>(static_cast<double>(imageCount) * animPercent), 0, imageCount - 1);
		const ObjectTextureID newObjectTextureID = animInst.objectTextureIDs.get(animIndex);

		DebugAssertIndex(this->objectInsts, animInst.objectIndex);
		ObjectInstance &objectInst = this->objectInsts[animInst.objectIndex];
		objectInst.setObjectTextureID(newObjectTextureID);
	}

	const Matrix4d timeOfDayRotation = RendererUtils::getTimeOfDayRotation(daytimePercent);
	const Matrix4d latitudeRotation = RendererUtils::getLatitudeRotation(latitude);

	auto transformObjectsInRange = [this, &timeOfDayRotation, &latitudeRotation](int start, int end)
	{
		for (int i = start; i < end; i++)
		{
			DebugAssertIndex(this->objectInsts, i);
			ObjectInstance &objectInst = this->objectInsts[i];
			const Double3 baseDirection = objectInst.getBaseDirection();
			Double4 dir(baseDirection.x, baseDirection.y, baseDirection.z, 0.0);
			dir = timeOfDayRotation * dir;
			dir = latitudeRotation * dir;

			// @temp: flip X and Z.
			// @todo: figure out why. Distant stars should rotate counter-clockwise when facing south,
			// and the sun and moons should rise from the west.
			objectInst.setTransformedDirection(Double3(-dir.x, dir.y, -dir.z));
		}
	};

	// Update transformed sky positions of moons, suns, and stars.
	transformObjectsInRange(this->moonStart, this->moonEnd);
	transformObjectsInRange(this->sunStart, this->sunEnd);
	transformObjectsInRange(this->starStart, this->starEnd);
}
