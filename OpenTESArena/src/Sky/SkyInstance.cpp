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
#include "../Rendering/RendererUtils.h"
#include "../Weather/WeatherInstance.h"
#include "../World/MapDefinition.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"

SkyObjectInstance::SkyObjectInstance()
{
	this->width = 0.0;
	this->height = 0.0;
	this->textureType = static_cast<SkyObjectTextureType>(-1);
	this->textureAssetEntryID = -1;
	this->paletteIndexEntryID = -1;
	this->emissive = false;
	this->animIndex = -1;
	this->pivotType = static_cast<SkyObjectPivotType>(-1);
}

void SkyObjectInstance::initTextured(const Double3 &baseDirection, double width, double height, SkyObjectTextureAssetEntryID textureAssetEntryID, bool emissive, int animIndex)
{
	this->baseDirection = baseDirection;
	this->transformedDirection = Double3::Zero;
	this->width = width;
	this->height = height;
	
	this->textureType = SkyObjectTextureType::TextureAsset;
	this->textureAssetEntryID = textureAssetEntryID;

	this->emissive = emissive;
	this->animIndex = animIndex;
	this->pivotType = SkyObjectPivotType::Bottom;
}

void SkyObjectInstance::initTextured(const Double3 &baseDirection, double width, double height, SkyObjectTextureAssetEntryID textureAssetEntryID, bool emissive)
{
	this->initTextured(baseDirection, width, height, textureAssetEntryID, emissive, -1);
}

void SkyObjectInstance::initPaletteIndex(const Double3 &baseDirection, double width, double height, SkyObjectPaletteIndexEntryID paletteIndexEntryID, bool emissive)
{
	this->baseDirection = baseDirection;
	this->transformedDirection = Double3::Zero;
	this->width = width;
	this->height = height;

	this->textureType = SkyObjectTextureType::PaletteIndex;
	this->paletteIndexEntryID = paletteIndexEntryID;

	this->emissive = emissive;
	this->animIndex = -1;
	this->pivotType = SkyObjectPivotType::Center;
}

SkyObjectAnimationInstance::SkyObjectAnimationInstance()
{
	this->skyObjectIndex = -1;
	this->targetSeconds = 0.0;
	this->currentSeconds = 0.0;
	this->percentDone = 0.0;
}

void SkyObjectAnimationInstance::init(int skyObjectIndex, double targetSeconds)
{
	this->skyObjectIndex = skyObjectIndex;
	this->targetSeconds = targetSeconds;
	this->currentSeconds = 0.0;
	this->percentDone = 0.0;
}

bool SkyInstance::tryGetTextureAssetEntryID(BufferView<const TextureAsset> textureAssets, SkyObjectTextureAssetEntryID *outID) const
{
	for (int i = 0; i < static_cast<int>(this->textureAssetEntries.size()); i++)
	{
		const SkyObjectTextureAssetEntry &entry = this->textureAssetEntries[i];
		const BufferView<const TextureAsset> entryTextureAssets(entry.textureAssets);
		if (entryTextureAssets.getCount() != textureAssets.getCount())
		{
			continue;
		}

		bool success = true;
		for (int j = 0; j < textureAssets.getCount(); j++)
		{
			const TextureAsset &textureAsset = textureAssets[j];
			const TextureAsset &entryTextureAsset = entryTextureAssets[j];
			if (textureAsset != entryTextureAsset)
			{
				success = false;
				break;
			}
		}

		if (!success)
		{
			continue;
		}

		*outID = static_cast<SkyObjectTextureAssetEntryID>(i);
		return true;
	}

	return false;
}

bool SkyInstance::tryGetPaletteIndexEntryID(uint8_t paletteIndex, SkyObjectPaletteIndexEntryID *outID) const
{
	for (int i = 0; i < static_cast<int>(this->paletteIndexEntries.size()); i++)
	{
		const SkyObjectPaletteIndexEntry &entry = this->paletteIndexEntries[i];
		if (entry.paletteIndex != paletteIndex)
		{
			continue;
		}

		*outID = static_cast<SkyObjectPaletteIndexEntryID>(i);
		return true;
	}

	return false;
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

void SkyInstance::init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition, int currentDay,
	TextureManager &textureManager)
{
	auto addGeneralObjectInst = [this, &textureManager](const Double3 &baseDirection, BufferView<const TextureAsset> textureAssets, bool emissive, double animSeconds = 0.0)
	{
		const TextureAsset &firstTextureAsset = textureAssets.get(0);
		const std::optional<TextureFileMetadataID> metadataID = textureManager.tryGetMetadataID(firstTextureAsset.filename.c_str());
		if (!metadataID.has_value())
		{
			DebugLogError("Couldn't load first texture metadata \"" + firstTextureAsset.filename + "\" for sky object.");
			return;
		}

		const TextureFileMetadata &metadata = textureManager.getMetadataHandle(*metadataID);
		double width, height;
		SkyUtils::getSkyObjectDimensions(metadata.getWidth(0), metadata.getHeight(0), &width, &height);

		// Get or add texture entry ID.
		auto textureIter = std::find_if(this->textureAssetEntries.begin(), this->textureAssetEntries.end(),
			[&textureAssets](const SkyObjectTextureAssetEntry &entry)
		{
			BufferView<const TextureAsset> entryTextureAssets(entry.textureAssets);
			if (entryTextureAssets.getCount() != textureAssets.getCount())
			{
				return false;
			}

			for (int i = 0; i < entryTextureAssets.getCount(); i++)
			{
				if (entryTextureAssets[i] != textureAssets[i])
				{
					return false;
				}
			}

			return true;
		});

		if (textureIter == this->textureAssetEntries.end())
		{
			SkyObjectTextureAssetEntry entry;
			entry.textureAssets.init(textureAssets.getCount());
			for (int i = 0; i < textureAssets.getCount(); i++)
			{
				entry.textureAssets[i] = textureAssets[i];
			}

			this->textureAssetEntries.emplace_back(std::move(entry));
			textureIter = this->textureAssetEntries.end() - 1;
		}

		const SkyObjectTextureAssetEntryID textureAssetEntryID = static_cast<int>(std::distance(this->textureAssetEntries.begin(), textureIter));
		const bool hasAnimation = textureAssets.getCount() > 1;

		SkyObjectInstance skyObjectInst;
		if (hasAnimation)
		{
			const int skyObjectIndex = static_cast<int>(this->skyObjectInsts.size());
			const double targetSeconds = (animSeconds == 0.0) ? (static_cast<double>(textureAssets.getCount()) * ArenaSkyUtils::ANIMATED_LAND_SECONDS_PER_FRAME) : animSeconds;

			SkyObjectAnimationInstance animInst;
			animInst.init(skyObjectIndex, targetSeconds);
			this->animInsts.emplace_back(animInst);

			const int animIndex = static_cast<int>(this->animInsts.size()) - 1;
			skyObjectInst.initTextured(baseDirection, width, height, textureAssetEntryID, emissive, animIndex);
		}
		else
		{
			skyObjectInst.initTextured(baseDirection, width, height, textureAssetEntryID, emissive);
		}
		
		this->skyObjectInsts.emplace_back(std::move(skyObjectInst));
	};

	auto addSmallStarObjectInst = [this](const Double3 &baseDirection, uint8_t paletteIndex)
	{
		double width, height;
		SkyUtils::getSkyObjectDimensions(1, 1, &width, &height);

		// Get or add palette index entry ID.
		auto paletteIndexIter = std::find_if(this->paletteIndexEntries.begin(), this->paletteIndexEntries.end(),
			[paletteIndex](const SkyObjectPaletteIndexEntry &entry)
		{
			return entry.paletteIndex == paletteIndex;
		});

		if (paletteIndexIter == this->paletteIndexEntries.end())
		{
			SkyObjectPaletteIndexEntry entry;
			entry.paletteIndex = paletteIndex;
			this->paletteIndexEntries.emplace_back(std::move(entry));
			paletteIndexIter = this->paletteIndexEntries.end() - 1;
		}

		const SkyObjectPaletteIndexEntryID paletteIndexEntryID = static_cast<int>(std::distance(this->paletteIndexEntries.begin(), paletteIndexIter));
		constexpr bool emissive = true;

		SkyObjectInstance skyObjectInst;
		skyObjectInst.initPaletteIndex(baseDirection, width, height, paletteIndexEntryID, emissive);
		this->skyObjectInsts.emplace_back(std::move(skyObjectInst));
	};

	// Spawn all sky objects from the ready-to-bake format. Any animated objects start on their first frame.
	int landInstCount = 0;
	for (int i = 0; i < skyDefinition.getLandPlacementDefCount(); i++)
	{
		const SkyDefinition::LandPlacementDef &placementDef = skyDefinition.getLandPlacementDef(i);
		const SkyDefinition::LandDefID defID = placementDef.id;
		const SkyLandDefinition &skyLandDef = skyInfoDefinition.getLand(defID);
		BufferView<const TextureAsset> textureAssets(skyLandDef.textureAssets);

		for (const Radians position : placementDef.positions)
		{
			// Convert radians to direction.
			const Radians angleY = 0.0;
			const Double3 direction = SkyUtils::getSkyObjectDirection(position, angleY);
			const bool emissive = skyLandDef.shadingType == SkyLandShadingType::Bright;
			addGeneralObjectInst(direction, textureAssets, emissive);

			// Do position transform since it's only needed once at initialization for land objects.
			SkyObjectInstance &skyObjectInst = this->skyObjectInsts.back();
			skyObjectInst.transformedDirection = skyObjectInst.baseDirection;
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
		const TextureAsset &textureAsset = skyAirDef.textureAsset;

		for (const std::pair<Radians, Radians> &position : placementDef.positions)
		{
			// Convert X and Y radians to direction.
			const Radians angleX = position.first;
			const Radians angleY = position.second;
			const Double3 direction = SkyUtils::getSkyObjectDirection(angleX, angleY);
			constexpr bool emissive = false;
			addGeneralObjectInst(direction, BufferView<const TextureAsset>(&textureAsset, 1), emissive);

			// Do position transform since it's only needed once at initialization for air objects.
			SkyObjectInstance &skyObjectInst = this->skyObjectInsts.back();
			skyObjectInst.transformedDirection = skyObjectInst.baseDirection;
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
		DebugAssert(skyMoonDef.textureAssets.getCount() > 0);
		const TextureAsset &textureAsset = skyMoonDef.textureAssets.get(currentDay);

		for (const SkyDefinition::MoonPlacementDef::Position &position : placementDef.positions)
		{
			// Default to the direction at midnight here, biased by the moon's bonus latitude and orbit percent.
			// @todo: not sure this matches the original game but it looks fine.
			const Matrix4d moonLatitudeRotation = RendererUtils::getLatitudeRotation(position.bonusLatitude);
			const Matrix4d moonOrbitPercentRotation = Matrix4d::xRotation(position.orbitPercent * Constants::TwoPi);
			const Double3 baseDirection = -Double3::UnitY;
			Double4 direction4D(baseDirection.x, baseDirection.y, baseDirection.z, 0.0);
			direction4D = moonLatitudeRotation * direction4D;
			direction4D = moonOrbitPercentRotation * direction4D;

			constexpr bool emissive = true;
			addGeneralObjectInst(Double3(direction4D.x, direction4D.y, direction4D.z), BufferView<const TextureAsset>(&textureAsset, 1), emissive);
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
		const TextureAsset &textureAsset = skySunDef.textureAsset;

		for (const double position : placementDef.positions)
		{
			// Default to the direction at midnight here, biased by the sun's bonus latitude.
			const Matrix4d sunLatitudeRotation = RendererUtils::getLatitudeRotation(position);
			const Double3 baseDirection = -Double3::UnitY;
			const Double4 direction4D = sunLatitudeRotation * Double4(baseDirection.x, baseDirection.y, baseDirection.z, 0.0);
			constexpr bool emissive = true;
			addGeneralObjectInst(Double3(direction4D.x, direction4D.y, direction4D.z), BufferView<const TextureAsset>(&textureAsset, 1), emissive);
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
		const SkyStarType starType = skyStarDef.type;

		if (starType == SkyStarType::Small)
		{
			// Small stars are 1x1 pixels.
			const SkySmallStarDefinition &smallStar = skyStarDef.smallStar;
			const uint8_t paletteIndex = smallStar.paletteIndex;

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				addSmallStarObjectInst(position, paletteIndex);
			}
		}
		else if (starType == SkyStarType::Large)
		{
			const SkyLargeStarDefinition &largeStar = skyStarDef.largeStar;
			const TextureAsset &textureAsset = largeStar.textureAsset;

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				constexpr bool emissive = true;
				addGeneralObjectInst(position, BufferView<const TextureAsset>(&textureAsset, 1), emissive);
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
			addGeneralObjectInst(Double3::Zero, skyLightningDef.textureAssets, true, skyLightningDef.animSeconds);

			this->lightningAnimIndices.set(i, static_cast<int>(this->animInsts.size()) - 1);
		}
	}

	this->lightningStart = this->starEnd;
	this->lightningEnd = this->lightningStart + lightningBoltDefCount;
}

const SkyObjectInstance &SkyInstance::getSkyObjectInst(int index) const
{
	return this->skyObjectInsts[index];
}

const SkyObjectAnimationInstance &SkyInstance::getAnimInst(int index) const
{
	return this->animInsts[index];
}

const SkyObjectTextureAssetEntry &SkyInstance::getTextureAssetEntry(SkyObjectTextureAssetEntryID id) const
{
	return this->textureAssetEntries[id];
}

const SkyObjectPaletteIndexEntry &SkyInstance::getPaletteIndexEntry(SkyObjectPaletteIndexEntryID id) const
{
	return this->paletteIndexEntries[id];
}

bool SkyInstance::isLightningVisible(int objectIndex) const
{
	return this->currentLightningBoltObjectIndex == objectIndex;
}

void SkyInstance::update(double dt, double latitude, double dayPercent, const WeatherInstance &weatherInst, Random &random)
{
	// Update lightning (if any).
	if (weatherInst.hasRain())
	{
		const WeatherRainInstance &rainInst = weatherInst.getRain();
		const std::optional<WeatherRainInstance::Thunderstorm> &thunderstorm = rainInst.thunderstorm;
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

				SkyObjectInstance &lightningObjInst = this->skyObjectInsts[*this->currentLightningBoltObjectIndex];
				lightningObjInst.transformedDirection = lightningDirection;
			}

			if (lightningBoltPercent.has_value())
			{
				const int animInstIndex = this->lightningAnimIndices.get(*this->currentLightningBoltObjectIndex - this->lightningStart);
				SkyObjectAnimationInstance &lightningAnimInst = this->animInsts[animInstIndex];
				lightningAnimInst.currentSeconds = *lightningBoltPercent * lightningAnimInst.targetSeconds;
				lightningAnimInst.percentDone = std::clamp(lightningAnimInst.currentSeconds / lightningAnimInst.targetSeconds, 0.0, 1.0);
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
		SkyObjectAnimationInstance &animInst = this->animInsts[i];

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

		if (isLightningAnim && !this->isLightningVisible(animInst.skyObjectIndex))
		{
			continue;
		}

		animInst.currentSeconds += dt;
		if (animInst.currentSeconds >= animInst.targetSeconds)
		{
			animInst.currentSeconds = std::fmod(animInst.currentSeconds, animInst.targetSeconds);
		}

		animInst.percentDone = std::clamp(animInst.currentSeconds / animInst.targetSeconds, 0.0, 1.0);
	}

	const Matrix4d timeOfDayRotation = RendererUtils::getTimeOfDayRotation(dayPercent);
	const Matrix4d latitudeRotation = RendererUtils::getLatitudeRotation(latitude);

	auto transformObjectsInRange = [this, &timeOfDayRotation, &latitudeRotation](int start, int end)
	{
		for (int i = start; i < end; i++)
		{
			DebugAssertIndex(this->skyObjectInsts, i);
			SkyObjectInstance &skyObjectInst = this->skyObjectInsts[i];
			const Double3 baseDirection = skyObjectInst.baseDirection;
			Double4 dir(baseDirection.x, baseDirection.y, baseDirection.z, 0.0);
			dir = timeOfDayRotation * dir;
			dir = latitudeRotation * dir;

			// @temp: flip X and Z.
			// @todo: figure out why. Distant stars should rotate counter-clockwise when facing south,
			// and the sun and moons should rise from the west.
			skyObjectInst.transformedDirection = Double3(-dir.x, dir.y, -dir.z);
		}
	};

	// Update transformed sky positions of moons, suns, and stars.
	transformObjectsInRange(this->moonStart, this->moonEnd);
	transformObjectsInRange(this->sunStart, this->sunEnd);
	transformObjectsInRange(this->starStart, this->starEnd);
}

void SkyInstance::clear()
{
	this->textureAssetEntries.clear();
	this->paletteIndexEntries.clear();
	this->skyObjectInsts.clear();
	this->animInsts.clear();
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
