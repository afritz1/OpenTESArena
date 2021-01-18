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
#include "../Media/TextureBuilder.h"
#include "../Media/TextureManager.h"

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
	TextureBuilderID textureBuilderID)
{
	this->init(ObjectInstance::Type::General, baseDirection, width, height);
	this->general.textureBuilderID = textureBuilderID;
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

SkyInstance::AnimInstance::AnimInstance(int objectIndex, const TextureBuilderIdGroup &textureBuilderIDs,
	double targetSeconds)
{
	this->objectIndex = objectIndex;
	this->textureBuilderIDs = textureBuilderIDs;
	this->targetSeconds = targetSeconds;
	this->currentSeconds = 0.0;
}

void SkyInstance::init(const SkyDefinition &skyDefinition, const SkyInfoDefinition &skyInfoDefinition,
	TextureManager &textureManager)
{
	auto addGeneralObjectInst = [this](const Double3 &baseDirection, double width, double height,
		TextureBuilderID textureBuilderID)
	{
		ObjectInstance objectInst;
		objectInst.initGeneral(baseDirection, width, height, textureBuilderID);
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
			addGeneralObjectInst(direction, width, height, *textureBuilderID);

			// Only land objects support animations (for now).
			if (skyLandDef.hasAnimation())
			{
				const int objectIndex = static_cast<int>(this->objectInsts.size()) - 1;
				// @todo: these texture builder IDs should come from iterating the SkyLandDefinition's texture asset refs.
				const TextureBuilderIdGroup textureBuilderIDs(*textureBuilderID, skyLandDef.getTextureCount());
				const double targetSeconds = static_cast<int>(static_cast<double>(textureBuilderIDs.getCount()) *
					ArenaSkyUtils::ANIMATED_LAND_SECONDS_PER_FRAME);
				addAnimInst(objectIndex, textureBuilderIDs, targetSeconds);
			}
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
			addGeneralObjectInst(direction, width, height, *textureBuilderID);
		}

		airInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->airStart = this->landEnd;
	this->airEnd = this->airStart + airInstCount;

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
				addGeneralObjectInst(position, width, height, *textureBuilderID);
			}
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(starType)));
		}

		starInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->starStart = this->airEnd;
	this->starEnd = this->starStart + starInstCount;

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
			// Convert starting sun latitude to direction.
			// @todo: just use fixed direction for now, see RendererUtils later.
			const Double3 tempDirection = Double3::UnitZ; // Temp: west. Ideally this would be -Y and rotated around +X (south).
			addGeneralObjectInst(tempDirection, width, height, *textureBuilderID);
		}

		sunInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->sunStart = this->starEnd;
	this->sunEnd = this->sunStart + sunInstCount;

	int moonInstCount = 0;
	for (int i = 0; i < skyDefinition.getMoonPlacementDefCount(); i++)
	{
		const SkyDefinition::MoonPlacementDef &placementDef = skyDefinition.getMoonPlacementDef(i);
		const SkyDefinition::MoonDefID defID = placementDef.id;
		const SkyMoonDefinition &skyMoonDef = skyInfoDefinition.getMoon(defID);

		// @todo: get the image from the current day, etc..
		DebugAssert(skyMoonDef.getTextureCount() > 0);
		const TextureAssetReference &textureAssetRef = skyMoonDef.getTextureAssetRef(0);
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

		for (const SkyDefinition::MoonPlacementDef::Position &position : placementDef.positions)
		{
			// Convert moon position to direction.
			// @todo: just use fixed direction for now, see RendererUtils later.
			const Double3 tempDirection = Double3::UnitZ; // Temp: west. Ideally this would be -Y and rotated around +X (south).
			addGeneralObjectInst(tempDirection, width, height, *textureBuilderID);
		}

		moonInstCount += static_cast<int>(placementDef.positions.size());
	}

	this->moonStart = this->sunEnd;
	this->moonEnd = this->moonStart + moonInstCount;
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

bool SkyInstance::isObjectSmallStar(int objectIndex) const
{
	DebugAssertIndex(this->objectInsts, objectIndex);
	return this->objectInsts[objectIndex].getType() != SkyInstance::ObjectInstance::Type::SmallStar;
}

void SkyInstance::getObject(int index, Double3 *outDirection, TextureBuilderID *outTextureBuilderID,
	double *outWidth, double *outHeight) const
{
	DebugAssertIndex(this->objectInsts, index);
	const ObjectInstance &objectInst = this->objectInsts[index];
	*outDirection = objectInst.getTransformedDirection();

	DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::General);
	*outTextureBuilderID = objectInst.getGeneral().textureBuilderID;

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

void SkyInstance::update(double dt, double latitude, double daytimePercent)
{
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

		animInst.currentSeconds += dt;
		if (animInst.currentSeconds >= animInst.targetSeconds)
		{
			animInst.currentSeconds = std::fmod(animInst.currentSeconds, animInst.targetSeconds);
		}

		const int imageCount = animInst.textureBuilderIDs.getCount();
		const double animPercent = std::clamp(animInst.currentSeconds / animInst.targetSeconds, 0.0, 1.0);
		const int animIndex = static_cast<int>(static_cast<double>(imageCount) * animPercent);
		const TextureBuilderID newTextureBuilderID = animInst.textureBuilderIDs.getID(animIndex);
		
		DebugAssertIndex(this->objectInsts, animInst.objectIndex);
		ObjectInstance &objectInst = this->objectInsts[animInst.objectIndex];

		DebugAssert(objectInst.getType() == SkyInstance::ObjectInstance::Type::General);
		ObjectInstance::General &objectInstGeneral = objectInst.getGeneral();
		objectInstGeneral.textureBuilderID = newTextureBuilderID;
	}

	// Update transformed sky position of stars, suns, and moons.
	for (int i = this->starStart; i < this->starEnd; i++)
	{
		DebugAssertIndex(this->objectInsts, i);
		ObjectInstance &objectInst = this->objectInsts[i];
		// @todo: actually transform direction based on latitude and time of day.
		const Double3 transformedDirection = objectInst.getBaseDirection();
		objectInst.setTransformedDirection(transformedDirection);
	}

	for (int i = this->sunStart; i < this->sunEnd; i++)
	{
		DebugAssertIndex(this->objectInsts, i);
		ObjectInstance &objectInst = this->objectInsts[i];
		// @todo: actually transform direction based on latitude and time of day.
		const Double3 transformedDirection = objectInst.getBaseDirection();
		objectInst.setTransformedDirection(transformedDirection);
	}

	for (int i = this->moonStart; i < this->moonEnd; i++)
	{
		DebugAssertIndex(this->objectInsts, i);
		ObjectInstance &objectInst = this->objectInsts[i];
		// @todo: actually transform direction based on latitude and time of day.
		const Double3 transformedDirection = objectInst.getBaseDirection();
		objectInst.setTransformedDirection(transformedDirection);
	}
}
