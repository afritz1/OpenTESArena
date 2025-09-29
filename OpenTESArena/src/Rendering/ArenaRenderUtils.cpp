#include <algorithm>
#include <optional>

#include "ArenaRenderUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureManager.h"
#include "../Interface/GameWorldUiModel.h"
#include "../Math/ArenaMathUtils.h"
#include "../Math/Constants.h"
#include "../Math/Random.h"
#include "../Math/Vector4.h"
#include "../Time/ArenaClockUtils.h"
#include "../Time/ClockLibrary.h"
#include "../World/MapType.h"

#include "components/debug/Debug.h"
#include "components/utilities/Bytes.h"

namespace
{
	Span<const int8_t> FOGLGT;
	Span<const int16_t> FOGTXT;

	constexpr int FogColumns = 40;
	constexpr int FogRows = 25;
	constexpr int FogTxtSampleBaseCount = FogColumns * FogRows;
	constexpr int FogTxtSampleExtraCount = 45;
	constexpr int FogTxtSampleTotalCount = FogTxtSampleBaseCount + FogTxtSampleExtraCount;
	int16_t FOGTXTSample[FogTxtSampleTotalCount];

	constexpr int ESWidth = ArenaRenderUtils::SCREEN_WIDTH;
	constexpr int ESHeight = ArenaRenderUtils::SCENE_VIEW_HEIGHT - 1;
	constexpr int ESElementCount = (ESWidth * ESHeight) / 2;
	int16_t ESArray[ESElementCount]; // For 320 columns x 146 rows of screen pixels.

	int16_t PlayerX = 0;
	int16_t PlayerZ = 0;
	int16_t PlayerAngle = 0;

	uint16_t WORD_4b80_191b; // Increases +4 every animation tick.
	uint16_t WORD_4b80_191d; // Increases +4 every animation tick.

	constexpr int32_t DWORD_4b80_819a = 0xD0300000; // Original game does a few calculations here to get the value, but it will always be this result
	int32_t DWORD_4b80_819e = 0;
	int32_t DWORD_4b80_81a2 = 0;
	constexpr int32_t DWORD_4b80_81a6 = 0x6906904; // Constant value. @476D6.
	constexpr int32_t DWORD_4b80_81aa = 0xDD5D5D5E; // Original game does a few calculations here to get the value, but it will always be this result
	uint16_t WORD_4b80_81ae = 0;
	uint16_t WORD_4b80_81b0 = 0;
	int16_t WORD_4b80_81b2 = 0;
	int16_t WORD_4b80_81b4 = 0;
	uint16_t WORD_4b80_81b6 = 0;
	uint16_t WORD_4b80_81b8 = 0;
	int16_t WORD_4b80_81c6 = 0;
	int16_t WORD_4b80_81c8 = 0;
	int16_t WORD_4b80_81ca = 0;
	constexpr int16_t WORD_4b80_81d4 = 0xFC00; // Constant value. @47704.
	int16_t WORD_ARRAY_4b80_81d8[24]; // @47708. Both read from and written to.
	int16_t WORD_4b80_8208 = 0; // Aaron: Likely current tile row (Y value)

	constexpr int16_t WORD_4b80_a76a = 0x533C; // This is variable, but in testing it was 0x533C, which matched the location (533C:0000) put in ES and represented here as  "ESArray". It might always be that when this function is called.
	constexpr int16_t WORD_4b80_a784 = 0x92; // Variable, but might always be 0x92 when fog functions called.
	int32_t DWORD_VALUE1 = 0;
	int32_t DWORD_VALUE2 = 0;
	int32_t DWORD_VALUE3 = 0;
	int32_t DWORD_VALUE4 = 0;
	int32_t DWORD_VALUE5 = 0;

	int8_t CL = 0;

	int16_t AX = 0;
	int16_t BX = 0;
	int16_t CX = 0;
	int16_t DX = 0;
	uint16_t DI = 0;
	int16_t BP = 0;
	int16_t SI = 0;
	int16_t ES = 0;

	int32_t EAX = 0;
	int32_t EBX = 0;
	int32_t ECX = 0;
	int32_t EDX = 0;
	int32_t EBP = 0;

	int64_t EAXEDX = 0;

	void SampleFOGTXT(Span<const int16_t> cosineTable)
	{
		int32_t index = 0;
		int32_t loopCount = 4;
		int32_t FOGTXTSampleIndex = 45;

		do
		{
			AX = WORD_ARRAY_4b80_81d8[index];
			CX = WORD_ARRAY_4b80_81d8[index + 2];
			DI = 511;
			DI -= PlayerAngle; // PlayerAngle is never greater than 511

			ArenaMathUtils::rotatePoint(DI, AX, CX, cosineTable);

			BX = WORD_ARRAY_4b80_81d8[index + 1];
			WORD_ARRAY_4b80_81d8[index + 3] = AX;

			WORD_ARRAY_4b80_81d8[index + 4] = BX;
			WORD_ARRAY_4b80_81d8[index + 5] = CX;
			index += 6;
			loopCount--;
		} while (loopCount != 0);

		WORD_4b80_8208 = 0;

		do
		{
			BP = WORD_4b80_a784; // Seems to always be 0092 when this function is called
			BP >>= 3;

			AX = WORD_ARRAY_4b80_81d8[15];
			AX -= WORD_ARRAY_4b80_81d8[3];

			int32_t product = AX * WORD_4b80_8208;
			AX = static_cast<int16_t>(product);
			DX = product >> 16;

			int32_t dividend = (DX << 16) | AX;
			int16_t divisor = BP;

			AX = static_cast<int16_t>(dividend / divisor);
			DX = dividend % divisor;

			AX += WORD_ARRAY_4b80_81d8[3];
			WORD_4b80_81b0 = AX;

			AX = WORD_ARRAY_4b80_81d8[16];
			AX -= WORD_ARRAY_4b80_81d8[4];

			product = AX * WORD_4b80_8208;
			AX = static_cast<int16_t>(product);
			DX = product >> 16;

			dividend = (DX << 16) | AX;
			divisor = BP;

			AX = static_cast<int16_t>(dividend / divisor);
			DX = dividend % divisor;

			AX += WORD_ARRAY_4b80_81d8[4];
			WORD_4b80_81b4 = AX;

			AX = WORD_ARRAY_4b80_81d8[17];
			AX -= WORD_ARRAY_4b80_81d8[5];

			product = AX * WORD_4b80_8208;
			AX = static_cast<int16_t>(product);
			DX = product >> 16;

			dividend = (DX << 16) | AX;
			divisor = BP;

			AX = static_cast<int16_t>(dividend / divisor);
			DX = dividend % divisor;

			AX += WORD_ARRAY_4b80_81d8[5];
			WORD_4b80_81b8 = AX;

			WORD_4b80_81ae = 0;
			WORD_4b80_81b2 = 0;
			WORD_4b80_81b6 = 0;

			AX = WORD_ARRAY_4b80_81d8[21];
			AX -= WORD_ARRAY_4b80_81d8[9];

			product = AX * WORD_4b80_8208;
			AX = static_cast<int16_t>(product);
			DX = product >> 16;

			dividend = (DX << 16) | AX;
			divisor = BP;

			AX = static_cast<int16_t>(dividend / divisor);
			DX = dividend % divisor;

			AX += WORD_ARRAY_4b80_81d8[9];
			WORD_4b80_81c6 = AX;

			AX = WORD_ARRAY_4b80_81d8[22];
			AX -= WORD_ARRAY_4b80_81d8[10];

			product = AX * WORD_4b80_8208;
			AX = static_cast<int16_t>(product);
			DX = product >> 16;

			dividend = (DX << 16) | AX;
			divisor = BP;

			AX = static_cast<int16_t>(dividend / divisor);
			DX = dividend % divisor;

			AX += WORD_ARRAY_4b80_81d8[10];
			WORD_4b80_81c8 = AX;

			AX = WORD_ARRAY_4b80_81d8[23];
			AX -= WORD_ARRAY_4b80_81d8[11];

			product = AX * WORD_4b80_8208;
			AX = static_cast<int16_t>(product);
			DX = product >> 16;

			dividend = (DX << 16) | AX;
			divisor = BP;

			AX = static_cast<int16_t>(dividend / divisor);
			DX = dividend % divisor;

			AX += WORD_ARRAY_4b80_81d8[11];
			WORD_4b80_81ca = AX;

			BP = 39;

			AX = WORD_4b80_81c6;
			AX -= WORD_4b80_81b0;
			EAX = AX << 16;

			int64_t dividend2 = EAX;
			int32_t divisor2 = BP;

			EAX = static_cast<int32_t>(dividend2 / divisor2);
			EDX = static_cast<int32_t>(dividend2 % divisor2);

			DWORD_VALUE3 = EAX;

			AX = WORD_4b80_81c8;
			AX -= WORD_4b80_81b4;
			EAX = AX << 16;

			dividend2 = EAX;
			divisor2 = BP;

			EAX = static_cast<int32_t>(dividend2 / divisor2);
			EDX = static_cast<int32_t>(dividend2 % divisor2);

			DWORD_VALUE4 = EAX;

			AX = WORD_4b80_81ca;
			AX -= WORD_4b80_81b8;
			EAX = AX << 16;

			dividend2 = EAX;
			divisor2 = BP;

			EAX = static_cast<int32_t>(dividend2 / divisor2);
			EDX = static_cast<int32_t>(dividend2 % divisor2);

			DWORD_VALUE5 = EAX;

			ECX = WORD_4b80_81b4 * WORD_4b80_81d4;
			DWORD_4b80_819e = ECX;

			EAX = WORD_4b80_81c8 * WORD_4b80_81d4;
			EAX -= ECX;

			int64_t product2 = static_cast<int64_t>(EAX) * static_cast<int64_t>(DWORD_4b80_81a6);
			EAX = static_cast<int32_t>(product2);
			EDX = product2 >> 32;

			DWORD_VALUE1 = EAX;
			DWORD_VALUE2 = EDX;
			DWORD_4b80_81a2 = 0;

			loopCount = 40;

			do
			{
				EAX = DWORD_4b80_819a;
				EBP = DWORD_4b80_819e;
				if (EBP != 0)
				{

					dividend2 = EAX;
					divisor2 = EBP;

					EAX = static_cast<int32_t>(dividend2 / divisor2);
					EDX = dividend2 % divisor2;

					if (EAX < 0)
					{
						product2 = static_cast<int64_t>(EAX) * static_cast<int64_t>(DWORD_4b80_81aa);
						EAX = static_cast<int32_t>(product2);
						EDX = product2 >> 32;
						EAX = (static_cast<uint32_t>(EAX) >> 31) | (EDX << 1);
					}

					EBX = EAX;
					EBP = WORD_4b80_81ae | (WORD_4b80_81b0 << 16);

					product2 = static_cast<int64_t>(EAX) * static_cast<int64_t>(EBP);
					EAX = static_cast<int32_t>(product2);
					EDX = product2 >> 32;

					EAX = (static_cast<uint32_t>(EAX) >> 24) | (EDX << 8);
					EAX += PlayerX + WORD_4b80_191b;
					EAX >>= 6;
					std::swap(EAX, EBX);

					EBP = WORD_4b80_81b6 | (WORD_4b80_81b8 << 16);

					product2 = static_cast<int64_t>(EAX) * static_cast<int64_t>(EBP);
					EAX = static_cast<int32_t>(product2);
					EDX = product2 >> 32;

					EAX = (static_cast<uint32_t>(EAX) >> 24) | (EDX << 8);
					EAX += PlayerZ + WORD_4b80_191d;
					EAX >>= 6;

					BX = static_cast<int16_t>(EBX);
					BX &= 0x7F;
					BX <<= 7;

					AX = static_cast<int16_t>(EAX);
					AX &= 0x7F;

					BX += AX;

					AX = FOGTXT[BX];
				}
				else
				{
					AX = (EAX & 0x00FF) | 0x0C00;
				}

				// Write the value to the sample buffer FOGTXTSample, a short value array, at FOGTXTSampleIndex.
				FOGTXTSample[FOGTXTSampleIndex] = AX; // Write the calculated value
				FOGTXTSampleIndex++;

				// Next is, in assembly:
				// ADD dword[81a2], DWORD_VALUE1
				// ADC dword[819e], DWORD_VALUE2
				// ADD dword[81ae], DWORD_VALUE3
				// ADD dword[81b2], DWORD_VALUE4
				// ADD dword[81b6], DWORD_VALUE5

				// The ADC instruction adds any carry from the preceding ADD instruction, so we need to get the carry
				bool carry = false;
				uint32_t before = DWORD_4b80_81a2;
				DWORD_4b80_81a2 += DWORD_VALUE1;
				if (DWORD_4b80_81a2 < before)
				{
					carry = true; // overflow occurred
				}

				DWORD_4b80_819e += DWORD_VALUE2;
				if (carry)
				{
					DWORD_4b80_819e++;
				}

				int sum = (WORD_4b80_81ae & 0xFFFF | ((WORD_4b80_81b0 & 0xFFFF) << 16));
				sum += DWORD_VALUE3;
				WORD_4b80_81ae = static_cast<int16_t>(sum);
				WORD_4b80_81b0 = sum >> 16;

				sum = (WORD_4b80_81b2 & 0xFFFF | ((WORD_4b80_81b4 & 0xFFFF) << 16));
				sum += DWORD_VALUE4;
				WORD_4b80_81b2 = static_cast<int16_t>(sum);
				WORD_4b80_81b4 = sum >> 16;

				sum = (WORD_4b80_81b6 & 0xFFFF | ((WORD_4b80_81b8 & 0xFFFF) << 16));
				sum += DWORD_VALUE5;
				WORD_4b80_81b6 = static_cast<int16_t>(sum);
				WORD_4b80_81b8 = sum >> 16;

				loopCount--;
			} while (loopCount != 0);

			WORD_4b80_8208++;
		} while (WORD_4b80_8208 != 25);
	}

	inline int8_t GetLightTableValue(int16_t index)
	{
		if (index < 0 || index >= FOGLGT.getCount())
		{
			index = 0; // Causes occasional black speckles
		}

		return FOGLGT[index];
	}

	inline void ApplyNewData()
	{
		BX += DX;
		CX = static_cast<int16_t>((CX & 0xFF) | ((BX & 0xFF) << 8));
		BX = (BX & 0xFF00) | (ESArray[DI / 2] & 0xFF);
		AX = static_cast<int16_t>((AX & 0xFF00) | GetLightTableValue(BX));
		BX = (CX & 0xFF00) >> 8;
		DX += BP;
		BX += DX;
		CX = static_cast<int16_t>((CX & 0xFF) | ((BX & 0xFF) << 8));
		BX = (BX & 0xFF00) | (ESArray[DI / 2] & 0xFF00) >> 8;
		AX = static_cast<int16_t>((AX & 0xFF) | (GetLightTableValue(BX) << 8));
		ESArray[DI / 2] = AX;
		DI += 2;
		BX = (CX & 0xFF00) >> 8;
		DX += BP;
	}

	inline void IterateOverData()
	{
		DX = FOGTXTSample[SI / 2];
		BP = FOGTXTSample[(SI + 2) / 2];
		BP -= DX;
		BP >>= 3;
		FOGTXTSample[SI / 2] = DX + FOGTXTSample[(SI - (FOGTXTSample[41] - 80)) / 2];
		ApplyNewData();
		ApplyNewData();
		ApplyNewData();
		ApplyNewData();
		SI++;
		SI++;
	}

	void ApplySampledFogData()
	{
		for (int32_t i = 0; i < 40; i++)
		{
			FOGTXTSample[405 + i] = 0;
		}

		FOGTXTSample[43] = WORD_4b80_a76a;
		FOGTXTSample[40] = (WORD_4b80_a784 + 7) >> 3;
		FOGTXTSample[41] = 90;
		FOGTXTSample[42] = 0;

		do
		{
			SI = FOGTXTSample[41] + 80;
			FOGTXTSample[41] = SI;
			for (int32_t i = 0; i < 40; i++)
			{
				FOGTXTSample[0 + i] = (FOGTXTSample[(SI / 2) + i] - FOGTXTSample[(SI / 2) - 40 + i]) >> 3;
			}

			DI = FOGTXTSample[42];
			ES = FOGTXTSample[43]; // 0x533C in testing, used for location of ESArray
			CX = 8;

			if (FOGTXTSample[40] == 1)
			{
				CX -= 6;
			}

			do
			{
				SI = FOGTXTSample[41] - 80;
				BX = 0;

				for (int32_t i = 0; i < 40; i++)
				{
					IterateOverData();
				}

				CL = (CX & 0xFF);
				CL--;
				CX = static_cast<int16_t>((CX & 0xFF00) | CL);
			} while (CL != 0);

			FOGTXTSample[42] = DI;
			FOGTXTSample[40]--;
		} while (FOGTXTSample[40] != 0);
	}
}

ArenaFogState::ArenaFogState()
{
	this->playerX = 0;
	this->playerZ = 0;
	this->playerAngle = 0;
	this->animOffset = 4;
	this->currentSeconds = 0.0;
}

void ArenaFogState::init(TextureManager &textureManager)
{
	constexpr const char fogTxtFilename[] = "FOG.TXT";
	const std::optional<TextureBuilderID> fogTxtTextureBuilderID = textureManager.tryGetTextureBuilderID(fogTxtFilename);
	if (!fogTxtTextureBuilderID.has_value())
	{
		DebugLogErrorFormat("Couldn't get fog texture builder ID for \"%s\".", fogTxtFilename);
		return;
	}

	const TextureBuilder &fogTxtTextureBuilder = textureManager.getTextureBuilderHandle(*fogTxtTextureBuilderID);
	const Span2D<const uint16_t> srcFogTxtTexels = fogTxtTextureBuilder.getTexels16();

	const int fogTxtTexelCount = srcFogTxtTexels.getWidth() * srcFogTxtTexels.getHeight();
	this->fogTxt.init(fogTxtTexelCount);
	std::copy(srcFogTxtTexels.begin(), srcFogTxtTexels.end(), this->fogTxt.begin());

	constexpr const char fogLgtFilename[] = "FOG.LGT";
	const std::optional<TextureBuilderID> fogLgtTextureBuilderID = textureManager.tryGetTextureBuilderID(fogLgtFilename);
	if (!fogLgtTextureBuilderID.has_value())
	{
		DebugLogErrorFormat("Couldn't get fog light texture builder ID for \"%s\".", fogLgtFilename);
		return;
	}

	const TextureBuilder &fogLgtTextureBuilder = textureManager.getTextureBuilderHandle(*fogLgtTextureBuilderID);
	const Span2D<const uint8_t> srcFogLgtTexels = fogLgtTextureBuilder.getTexels8();

	const int fogLgtTexelCount = srcFogLgtTexels.getWidth() * srcFogLgtTexels.getHeight();
	this->fogLgt.init(fogLgtTexelCount);
	std::copy(srcFogLgtTexels.begin(), srcFogLgtTexels.end(), this->fogLgt.begin());
}

void ArenaFogState::update(double dt, const WorldDouble3 &playerPos, const Double2 &playerDir, MapType mapType)
{
	constexpr int ArenaUnitsInteger = static_cast<int>(MIFUtils::ARENA_UNITS);
	const OriginalInt2 originalPlayerPos = GameWorldUiModel::getOriginalPlayerPositionArenaUnits(playerPos, mapType);
	this->playerX = originalPlayerPos.y;
	this->playerZ = originalPlayerPos.x;

	// 0 at due south
	// 0x80 (128) at due west
	// 0x100 (256) at due north
	// 0x180 (384) at due east
	// The maximum value it can hold is 0x1FF (511).
	auto getOriginalAngle = [](double x, double y)
	{
		const Radians baseAngleRadians = MathUtils::fullAtan2(y, x);
		const Radians transformedAngleRadians = std::fmod(-baseAngleRadians + (7.0 * Constants::Pi / 2.0), Constants::TwoPi); // Ugly but works
		const double anglePercent = transformedAngleRadians / Constants::TwoPi;
		return std::clamp<short>(static_cast<short>(anglePercent * 512.0), 0, 511);
	};

	this->playerAngle = getOriginalAngle(-playerDir.y, -playerDir.x);

	constexpr double FOG_SECONDS_PER_FRAME = 1.0 / 25.0;

	this->currentSeconds += dt;
	if (this->currentSeconds >= FOG_SECONDS_PER_FRAME)
	{
		this->currentSeconds = std::fmod(this->currentSeconds, FOG_SECONDS_PER_FRAME);
		this->animOffset += 4;
	}
}

double ArenaRenderUtils::getAmbientPercent(const Clock &clock, MapType mapType, bool isFoggy)
{
	if (mapType == MapType::Interior)
	{
		return 0.0;
	}
	else if ((mapType == MapType::City) || (mapType == MapType::Wilderness))
	{
		if (isFoggy)
		{
			return 0.0; // This assumes it is during the daytime.
		}

		const ClockLibrary &clockLibrary = ClockLibrary::getInstance();

		// Time ranges where the ambient light changes. The start times are inclusive, and the end times are exclusive.
		const Clock &startBrighteningClock = clockLibrary.getClock(ArenaClockUtils::AmbientBrighteningStart);
		const Clock &endBrighteningClock = clockLibrary.getClock(ArenaClockUtils::AmbientBrighteningEnd);
		const Clock &startDimmingClock = clockLibrary.getClock(ArenaClockUtils::AmbientDimmingStart);
		const Clock &endDimmingClock = clockLibrary.getClock(ArenaClockUtils::AmbientDimmingEnd);
		const double startBrighteningTime = startBrighteningClock.getTotalSeconds();
		const double endBrighteningTime = endBrighteningClock.getTotalSeconds();
		const double startDimmingTime = startDimmingClock.getTotalSeconds();
		const double endDimmingTime = endDimmingClock.getTotalSeconds();

		const double clockTime = clock.getTotalSeconds();

		constexpr double minAmbient = 0.0;
		constexpr double maxAmbient = 1.0;

		double ambient;
		if ((clockTime >= endBrighteningTime) && (clockTime < startDimmingTime))
		{
			// Daytime ambient.
			ambient = maxAmbient;
		}
		else if ((clockTime >= startBrighteningTime) && (clockTime < endBrighteningTime))
		{
			// Interpolate brightening light (in the morning).
			const double timePercent = (clockTime - startBrighteningTime) / (endBrighteningTime - startBrighteningTime);
			ambient = minAmbient + ((maxAmbient - minAmbient) * timePercent);
		}
		else if ((clockTime >= startDimmingTime) && (clockTime < endDimmingTime))
		{
			// Interpolate dimming light (in the evening).
			const double timePercent = (clockTime - startDimmingTime) / (endDimmingTime - startDimmingTime);
			ambient = maxAmbient + ((minAmbient - maxAmbient) * timePercent);
		}
		else
		{
			// Night ambient.
			ambient = minAmbient;
		}

		return std::clamp(ambient, minAmbient, maxAmbient);
	}
	else
	{
		DebugUnhandledReturnMsg(double, std::to_string(static_cast<int>(mapType)));
	}
}

double ArenaRenderUtils::getDistantAmbientPercent(const Clock &clock)
{
	constexpr MapType mapType = MapType::City;
	constexpr bool isFoggy = false;
	const double ambientPercent = ArenaRenderUtils::getAmbientPercent(clock, mapType, isFoggy);
	constexpr double minDistantAmbient = 0.10;
	constexpr double maxDistantAmbient = 1.0;
	return std::clamp(ambientPercent, minDistantAmbient, maxDistantAmbient);
}

bool ArenaRenderUtils::isLightLevelTexel(uint8_t texel)
{
	return (texel >= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_LOWEST) && (texel <= ArenaRenderUtils::PALETTE_INDEX_LIGHT_LEVEL_HIGHEST);
}

bool ArenaRenderUtils::isPuddleTexel(uint8_t texel)
{
	return (texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_EVEN_ROW) || (texel == ArenaRenderUtils::PALETTE_INDEX_PUDDLE_ODD_ROW);
}

void ArenaRenderUtils::populateFogTexture(const ArenaFogState &fogState, Span2D<uint8_t> outPixels)
{
	const BinaryAssetLibrary &binaryAssetLibrary = BinaryAssetLibrary::getInstance();
	const ExeData &exeData = binaryAssetLibrary.getExeData();
	const ExeDataWeather &exeDataWeather = exeData.weather;
	Span<const int16_t> cosineTable = exeData.math.cosineTable;

	FOGLGT = Span<const int8_t>(reinterpret_cast<const int8_t*>(fogState.fogLgt.begin()), fogState.fogLgt.getCount());
	FOGTXT = Span<const int16_t>(reinterpret_cast<const int16_t*>(fogState.fogTxt.begin()), fogState.fogTxt.getCount());

	std::copy(std::begin(exeDataWeather.fogTxtSampleHelper), std::end(exeDataWeather.fogTxtSampleHelper), std::begin(WORD_ARRAY_4b80_81d8));
	std::fill(std::begin(ESArray), std::end(ESArray), 0x2566);
	//std::fill(std::begin(ESArray), std::end(ESArray), 0);

	PlayerX = fogState.playerX;
	PlayerZ = fogState.playerZ;
	PlayerAngle = fogState.playerAngle;
	WORD_4b80_191b = fogState.animOffset;
	WORD_4b80_191d = fogState.animOffset;

	SampleFOGTXT(cosineTable);
	ApplySampledFogData();

	outPixels.fill(0);

	for (int y = 0; y < ESHeight; y++)
	{
		for (int x = 0; x < ESWidth; x++)
		{
			const int srcIndex = x + (y * ESWidth);
			DebugAssert(srcIndex < (ESWidth * ESHeight));
			const uint8_t *ESArrayPtr = reinterpret_cast<const uint8_t*>(ESArray);
			const uint8_t sample = ESArrayPtr[srcIndex];
			const uint8_t lightLevel = sample;
			outPixels.set(x, y, lightLevel);
		}
	}

	/*for (int row = 0; row < FogRows; row++)
	{
		for (int column = 0; column < FogColumns; column++)
		{
			const int srcIndex = column + (row * FogColumns) + FogTxtSampleExtraCount;
			const int16_t sample = FOGTXTSample[srcIndex];
			const uint8_t lightLevel = static_cast<uint8_t>(sample >> 8);
			const uint8_t dither = static_cast<uint8_t>(sample);

			constexpr int tileDimension = 8;
			for (int j = 0; j < tileDimension; j++)
			{
				for (int i = 0; i < tileDimension; i++)
				{
					const int x = (column * tileDimension) + i;
					const int y = (row * tileDimension) + j;

					const bool shouldDither = false;// (dither & (1 << i)) != 0;
					const uint8_t calculatedLightLevel = lightLevel + (shouldDither ? 1 : 0);
					outPixels.set(x, y, calculatedLightLevel);
				}
			}
		}
	}*/
}
