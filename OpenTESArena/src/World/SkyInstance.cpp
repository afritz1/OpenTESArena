#include <cmath>

#include "ArenaSkyUtils.h"
#include "MapDefinition.h"
#include "MapType.h"
#include "SkyAirDefinition.h"
#include "SkyDefinition.h"
#include "SkyInfoDefinition.h"
#include "SkyInstance.h"
#include "SkyLandDefinition.h"
#include "SkyMoonDefinition.h"
#include "SkyStarDefinition.h"
#include "SkySunDefinition.h"
#include "SkyUtils.h"
#include "WeatherInstance.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Math/Random.h"
#include "../Media/TextureBuilder.h"
#include "../Media/TextureManager.h"
#include "../Rendering/Renderer.h"
#include "../Rendering/RendererUtils.h"

#include "components/debug/Debug.h"

SkyInstance::ObjectInstance::ObjectInstance()
{
	this->type = static_cast<ObjectInstance::Type>(-1);
	this->width = 0.0;
	this->height = 0.0;
}

void SkyInstance::ObjectInstance::init(Type type, const Double3 &baseDirection, double width, double height)
{
	this->type = type;
	this->baseDirection = baseDirection;
	this->width = width;
	this->height = height;
}

void SkyInstance::ObjectInstance::initGeneral(const Double3 &baseDirection, double width, double height,
	TextureBuilderID textureBuilderID, bool emissive)
{
	this->init(ObjectInstance::Type::General, baseDirection, width, height);
	this->general.textureBuilderID = textureBuilderID;
	this->general.emissive = emissive;
}

void SkyInstance::ObjectInstance::initSmallStar(const Double3 &baseDirection, double width, double height,
	uint8_t paletteIndex)
{
	this->init(ObjectInstance::Type::SmallStar, baseDirection, width, height);
	this->smallStar.paletteIndex = paletteIndex;
}

SkyInstance::ObjectInstance::Type SkyInstance::ObjectInstance::getType() const
{
	return this->type;
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

SkyInstance::ObjectInstance::General &SkyInstance::ObjectInstance::getGeneral()
{
	DebugAssert(this->type == ObjectInstance::Type::General);
	return this->general;
}

const SkyInstance::ObjectInstance::General &SkyInstance::ObjectInstance::getGeneral() const
{
	DebugAssert(this->type == ObjectInstance::Type::General);
	return this->general;
}

const SkyInstance::ObjectInstance::SmallStar &SkyInstance::ObjectInstance::getSmallStar() const
{
	DebugAssert(this->type == ObjectInstance::Type::SmallStar);
	return this->smallStar;
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

SkyInstance::AnimInstance::AnimInstance(int objectIndex, const TextureBuilderIdGroup &textureBuilderIDs,
	double targetSeconds)
{
	this->objectIndex = objectIndex;
	this->textureBuilderIDs = textureBuilderIDs;
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
	int currentDay, TextureManager &textureManager)
{
	auto addGeneralObjectInst = [this](const Double3 &baseDirection, double width, double height,
		TextureBuilderID textureBuilderID, bool emissive)
	{
		ObjectInstance objectInst;
		objectInst.initGeneral(baseDirection, width, height, textureBuilderID, emissive);
		this->objectInsts.emplace_back(std::move(objectInst));
	};

	auto addSmallStarObjectInst = [this](const Double3 &baseDirection, double width, double height,
		uint8_t paletteIndex)
	{
		ObjectInstance objectInst;
		objectInst.initSmallStar(baseDirection, width, height, paletteIndex);
		this->objectInsts.emplace_back(std::move(objectInst));
	};

	auto addAnimInst = [this](int objectIndex, const TextureBuilderIdGroup &textureBuilderIDs,
		double targetSeconds)
	{
		this->animInsts.emplace_back(objectIndex, textureBuilderIDs, targetSeconds);
	};

	// Spawn all sky objects from the ready-to-bake format. Any animated objects start on their first frame.
	int landInstCount = 0;
	for (int i = 0; i < skyDefinition.getLandPlacementDefCount(); i++)
	{
		const SkyDefinition::LandPlacementDef &placementDef = skyDefinition.getLandPlacementDef(i);
		const SkyDefinition::LandDefID defID = placementDef.id;
		const SkyLandDefinition &skyLandDef = skyInfoDefinition.getLand(defID);

		DebugAssert(skyLandDef.getTextureCount() > 0);
		const TextureAssetReference &textureAssetRef = skyLandDef.getTextureAssetRef(0);
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef);
		if (!textureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
			continue;
		}

		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(textureBuilder.getWidth(), textureBuilder.getHeight(), &width, &height);

		for (const Radians position : placementDef.positions)
		{
			// Convert radians to direction.
			const Radians angleY = 0.0;
			const Double3 direction = SkyUtils::getSkyObjectDirection(position, angleY);
			const bool emissive = skyLandDef.getShadingType() == SkyLandDefinition::ShadingType::Bright;
			addGeneralObjectInst(direction, width, height, *textureBuilderID, emissive);

			// Only land objects support animations (for now).
			if (skyLandDef.hasAnimation())
			{
				const int objectIndex = static_cast<int>(this->objectInsts.size()) - 1;
				// @todo: these texture builder IDs should come from iterating the SkyLandDefinition's texture asset refs.
				const TextureBuilderIdGroup textureBuilderIDs(*textureBuilderID, skyLandDef.getTextureCount());
				const double targetSeconds = static_cast<double>(textureBuilderIDs.getCount()) *
					ArenaSkyUtils::ANIMATED_LAND_SECONDS_PER_FRAME;
				addAnimInst(objectIndex, textureBuilderIDs, targetSeconds);
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
		const TextureAssetReference &textureAssetRef = skyAirDef.getTextureAssetRef();
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef);
		if (!textureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
			continue;
		}

		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(textureBuilder.getWidth(), textureBuilder.getHeight(), &width, &height);

		for (const std::pair<Radians, Radians> &position : placementDef.positions)
		{
			// Convert X and Y radians to direction.
			const Radians angleX = position.first;
			const Radians angleY = position.second;
			const Double3 direction = SkyUtils::getSkyObjectDirection(angleX, angleY);
			constexpr bool emissive = false;
			addGeneralObjectInst(direction, width, height, *textureBuilderID, emissive);

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
		const TextureAssetReference &textureAssetRef = skyMoonDef.getTextureAssetRef(currentDay);
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef);
		if (!textureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
			continue;
		}

		// @todo: maybe move this into the per-moon-position loop below.
		// @todo: use SkyDefinition::MoonPlacementDef::Position::imageIndex
		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(textureBuilder.getWidth(), textureBuilder.getHeight(), &width, &height);

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
			addGeneralObjectInst(Double3(direction4D.x, direction4D.y, direction4D.z), width, height,
				*textureBuilderID, emissive);
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
		const TextureAssetReference &textureAssetRef = skySunDef.getTextureAssetRef();
		const std::optional<TextureBuilderID> textureBuilderID =
			textureManager.tryGetTextureBuilderID(textureAssetRef);
		if (!textureBuilderID.has_value())
		{
			DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
			continue;
		}

		const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);

		double width, height;
		SkyUtils::getSkyObjectDimensions(textureBuilder.getWidth(), textureBuilder.getHeight(), &width, &height);

		for (const double position : placementDef.positions)
		{
			// Default to the direction at midnight here, biased by the sun's bonus latitude.
			const Matrix4d sunLatitudeRotation = RendererUtils::getLatitudeRotation(position);
			const Double3 baseDirection = -Double3::UnitY;
			const Double4 direction4D = sunLatitudeRotation *
				Double4(baseDirection.x, baseDirection.y, baseDirection.z, 0.0);
			constexpr bool emissive = true;
			addGeneralObjectInst(Double3(direction4D.x, direction4D.y, direction4D.z),
				width, height, *textureBuilderID, emissive);
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
			constexpr int imageWidth = 1;
			constexpr int imageHeight = imageWidth;

			double width, height;
			SkyUtils::getSkyObjectDimensions(imageWidth, imageHeight, &width, &height);

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				addSmallStarObjectInst(position, width, height, paletteIndex);
			}
		}
		else if (starType == SkyStarDefinition::Type::Large)
		{
			const SkyStarDefinition::LargeStar &largeStar = skyStarDef.getLargeStar();
			const TextureAssetReference &textureAssetRef = largeStar.textureAssetRef;
			const std::optional<TextureBuilderID> textureBuilderID =
				textureManager.tryGetTextureBuilderID(textureAssetRef);
			if (!textureBuilderID.has_value())
			{
				DebugLogError("Couldn't get texture builder ID for \"" + textureAssetRef.filename + "\".");
				continue;
			}

			const TextureBuilder &textureBuilder = textureManager.getTextureBuilderHandle(*textureBuilderID);

			double width, height;
			SkyUtils::getSkyObjectDimensions(textureBuilder.getWidth(), textureBuilder.getHeight(), &width, &height);

			for (const Double3 &position : placementDef.positions)
			{
				// Use star direction directly.
				constexpr bool emissive = true;
				addGeneralObjectInst(position, width, height, *textureBuilderID, emissive);
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
			const TextureBuilderIdGroup idGroup = [&textureManager, &skyLightningDef]()
			{
				DebugAssert(skyLightningDef.getTextureCount() > 0);
				const TextureAssetReference &firstTextureAssetRef = skyLightningDef.getTextureAssetRef(0);
				const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(firstTextureAssetRef);
				if (!textureBuilderID.has_value())
				{
					DebugLogError("Couldn't get texture builder ID for \"" + firstTextureAssetRef.filename + "\".");
					return TextureBuilderIdGroup();
				}

				// Load the remaining frames so they are all sequential in the texture manager.
				for (int frameIndex = 1; frameIndex < skyLightningDef.getTextureCount(); frameIndex++)
				{
					textureManager.tryGetTextureBuilderID(skyLightningDef.getTextureAssetRef(frameIndex));
				}

				return TextureBuilderIdGroup(*textureBuilderID, skyLightningDef.getTextureCount());
			}();

			const TextureBuilder &firstTextureBuilder = textureManager.getTextureBuilderHandle(idGroup.getID(0));
			double width, height;
			SkyUtils::getSkyObjectDimensions(firstTextureBuilder.getWidth(), firstTextureBuilder.getHeight(), &width, &height);

			addGeneralObjectInst(Double3::Zero, width, height, idGroup.getID(0), true);
			addAnimInst(static_cast<int>(this->objectInsts.size()) - 1, idGroup, skyLightningDef.getAnimationSeconds());

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

bool SkyInstance::isObjectSmallStar(int objectIndex) const
{
	DebugAssertIndex(this->objectInsts, objectIndex);
	const ObjectInstance &objectInst = this->objectInsts[objectIndex];
	return objectInst.getType() == SkyInstance::ObjectInstance::Type::SmallStar;
}

bool SkyInstance::isLightningVisible(int objectIndex) const
{
	return this->currentLightningBoltObjectIndex == objectIndex;
}

void SkyInstance::getObject(int index, Double3 *outDirection, TextureBuilderID *outTextureBuilderID,
	bool *outEmissive, double *outWidth, double *outHeight) const
{
	DebugAssertIndex(this->objectInsts, index);
	const ObjectInstance &objectInst = this->objectInsts[index];
	*outDirection = objectInst.getTransformedDirection();

	DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::General);
	const SkyInstance::ObjectInstance::General &generalInst = objectInst.getGeneral();
	*outTextureBuilderID = generalInst.textureBuilderID;
	*outEmissive = generalInst.emissive;

	*outWidth = objectInst.getWidth();
	*outHeight = objectInst.getHeight();
}

void SkyInstance::getObjectSmallStar(int index, Double3 *outDirection, uint8_t *outPaletteIndex,
	double *outWidth, double *outHeight) const
{
	DebugAssertIndex(this->objectInsts, index);
	const ObjectInstance &objectInst = this->objectInsts[index];
	*outDirection = objectInst.getTransformedDirection();

	DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::SmallStar);
	*outPaletteIndex = objectInst.getSmallStar().paletteIndex;

	*outWidth = objectInst.getWidth();
	*outHeight = objectInst.getHeight();
}

TextureBuilderIdGroup SkyInstance::getObjectTextureBuilderIDs(int index) const
{
	DebugAssert(!this->isObjectSmallStar(index));

	// See if there's an animation with the texture builder IDs.
	for (int i = 0; i < static_cast<int>(this->animInsts.size()); i++)
	{
		const SkyInstance::AnimInstance &animInst = this->animInsts[i];
		if (animInst.objectIndex == index)
		{
			return animInst.textureBuilderIDs;
		}
	}

	// Just get the object's texture builder ID directly.
	DebugAssertIndex(this->objectInsts, index);
	const SkyInstance::ObjectInstance &objectInst = this->objectInsts[index];
	const SkyInstance::ObjectInstance::General &generalInst = objectInst.getGeneral();
	return TextureBuilderIdGroup(generalInst.textureBuilderID, 1);
}

std::optional<double> SkyInstance::tryGetObjectAnimPercent(int index) const
{
	DebugAssert(!this->isObjectSmallStar(index));

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

bool SkyInstance::trySetActive(const std::optional<int> &activeLevelIndex, const MapDefinition &mapDefinition,
	TextureManager &textureManager, Renderer &renderer)
{
	// Note: this method relies on LevelInstance::trySetActive() being called first (for texture clearing).
	// @todo: do we want separation of "level" and "sky" in the renderer or no?
	// - I.e. clearLevelTextures()/clearSkyTextures(). Might evolve into level/sky/UI.

	// Although the sky could be treated the same way as visible entities (regenerated every frame), keep it this
	// way because the sky is not added to and removed from like entities are. The sky is baked once per level
	// and that's it.
	
	// @todo: tell SceneGraph to clear its sky geometry
	DebugNotImplementedMsg("trySetActive");
	//renderer.clearSky();

	const MapType mapType = mapDefinition.getMapType();
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

	DebugNotImplementedMsg("trySetActive"); // @todo: give to SceneGraph to process into an object texture for RenderFrameSettings
	//renderer.setSkyColors(skyColors.get(), skyColors.getCount());

	// Set sky objects in the renderer.
	const std::string &paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette \"" + paletteName + "\".");
		return false;
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	DebugNotImplementedMsg("trySetActive"); // @todo: this should set all the distant sky intermediate values in SceneGraph I think.
	//renderer.setSky(*this, palette, textureManager);

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

		// Small stars don't have animations.
		if (this->isObjectSmallStar(animInst.objectIndex))
		{
			continue;
		}

		// Don't update if it's an inactive lightning bolt.
		const bool isLightningAnim = [this, i]()
		{
			if (this->lightningAnimIndices.getCount() == 0)
			{
				return false;
			}

			const auto iter = std::find(this->lightningAnimIndices.get(), this->lightningAnimIndices.end(), i);
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

		const int imageCount = animInst.textureBuilderIDs.getCount();
		const double animPercent = animInst.currentSeconds / animInst.targetSeconds;
		const int animIndex = std::clamp(
			static_cast<int>(static_cast<double>(imageCount) * animPercent), 0, imageCount - 1);
		const TextureBuilderID newTextureBuilderID = animInst.textureBuilderIDs.getID(animIndex);

		DebugAssertIndex(this->objectInsts, animInst.objectIndex);
		ObjectInstance &objectInst = this->objectInsts[animInst.objectIndex];

		DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::General);
		ObjectInstance::General &objectInstGeneral = objectInst.getGeneral();
		objectInstGeneral.textureBuilderID = newTextureBuilderID;
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
