#include <algorithm>
#include <optional>

#include "ArenaRenderUtils.h"
#include "../Assets/BinaryAssetLibrary.h"
#include "../Assets/ExeData.h"
#include "../Assets/TextureBuilder.h"
#include "../Assets/TextureManager.h"
#include "../Interface/GameWorldUiModel.h"
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
	constexpr int FogColumns = 40;
	constexpr int FogRows = 25;
	constexpr int FogTxtSampleBaseCount = FogColumns * FogRows;
	constexpr int FogTxtSampleExtraCount = 45;
	constexpr int FogTxtSampleTotalCount = FogTxtSampleBaseCount + FogTxtSampleExtraCount;
	uint16_t g_fogTxtSamples[FogTxtSampleTotalCount];

	constexpr int ESWidth = ArenaRenderUtils::SCREEN_WIDTH;
	constexpr int ESHeight = ArenaRenderUtils::SCENE_VIEW_HEIGHT - 1;
	constexpr int ESElementCount = (ESWidth * ESHeight) / 2;
	short g_ESArray[ESElementCount]; // For 320 columns x 146 rows of screen pixels (moved here to avoid stack warning).

	constexpr int DWORD_4b80_819a = 0xD0300000; // Original game does a few calculations here to get the value, but it will always be this result
	int DWORD_4b80_819e = 0;
	int DWORD_4b80_81a2 = 0;
	constexpr int DWORD_4b80_81a6 = 0x6906904; // Constant value. @476D6.
	constexpr int DWORD_4b80_81aa = 0xDD5D5D5E; // Original game does a few calculations here to get the value, but it will always be this result
	short WORD_4b80_81ae = 0;
	short WORD_4b80_81b0 = 0;
	short WORD_4b80_81b2 = 0;
	short WORD_4b80_81b4 = 0;
	short WORD_4b80_81b6 = 0;
	short WORD_4b80_81b8 = 0;
	short WORD_4b80_81c6 = 0;
	short WORD_4b80_81c8 = 0;
	short WORD_4b80_81ca = 0;
	constexpr short WORD_4b80_81d4 = 0xFC00; // Constant value. @47704.
	short WORD_ARRAY_4b80_81d8[24]; // @47708. Both read from and written to.
	short WORD_4b80_8208 = 0; // Aaron: Likely current tile row (Y value)

	constexpr short WORD_4b80_a784 = 0x92; // Variable value, but might always be 0x92 when this function is called.
	int DWORD_VALUE1 = 0;
	int DWORD_VALUE2 = 0;
	int DWORD_VALUE3 = 0;
	int DWORD_VALUE4 = 0;
	int DWORD_VALUE5 = 0;

	short AX = 0;
	short BX = 0;
	short CX = 0;
	short DI = 0;
	short DX = 0;
	short BP = 0;
	short SI = 0;
	short ES = 0;

	int EAX = 0;
	int EBX = 0;
	int ECX = 0;
	int EDX = 0;
	int EBP = 0;

	int64_t EAXEDX = 0;

	void FogRotateVector(int angle, short &x, short &y, const ExeDataMath &exeDataMath)
	{
		const Span<const int16_t> cosineTable = exeDataMath.cosineTable;
		const short cosAngleMultiplier = cosineTable[angle];
		const short sinAngleMultiplier = cosineTable[angle + 128];

		const short doubledX = x * 2;
		const short doubledY = y * 2;

		short negCosAngleMultipler = -cosAngleMultiplier;
		if (negCosAngleMultipler >= 0)
		{
			negCosAngleMultipler--;
		}

		const int imulRes1 = doubledX * sinAngleMultiplier;
		const int imulRes2 = doubledY * negCosAngleMultipler;
		const int imulRes3 = doubledX * cosAngleMultiplier;
		const int imulRes4 = doubledY * sinAngleMultiplier;

		const short highRes1 = static_cast<uint32_t>(imulRes1) >> 16;
		const short highRes2 = static_cast<uint32_t>(imulRes2) >> 16;
		const short highRes3 = static_cast<uint32_t>(imulRes3) >> 16;
		const short highRes4 = static_cast<uint32_t>(imulRes4) >> 16;

		x = highRes2 + highRes1;
		y = highRes3 + highRes4;
	}

	void SampleFOGTXT(const ArenaFogState &fogState, const ExeData &exeData)
	{
		std::fill(std::begin(g_fogTxtSamples), std::end(g_fogTxtSamples), 0);

		int loopCount = 4;
		int index = 0;

		do
		{
			AX = WORD_ARRAY_4b80_81d8[index];
			CX = WORD_ARRAY_4b80_81d8[index + 2];
			DI = 511;
			DI -= fogState.PlayerAngle; // PlayerAngle is never greater than 511

			FogRotateVector(DI, AX, CX, exeData.math);

			BX = WORD_ARRAY_4b80_81d8[index + 1];
			WORD_ARRAY_4b80_81d8[index + 3] = AX;

			WORD_ARRAY_4b80_81d8[index + 4] = BX;
			WORD_ARRAY_4b80_81d8[index + 5] = CX;
			index += 6;
			loopCount--;
		} while (loopCount != 0);

		WORD_4b80_8208 = 0;

		int fogTxtSampleIndex = FogTxtSampleExtraCount;

		do
		{
			BP = WORD_4b80_a784; // Seems to always be 0092 when this function is called
			BP >>= 3;

			AX = WORD_ARRAY_4b80_81d8[15];
			AX -= WORD_ARRAY_4b80_81d8[3];

			int product = AX * WORD_4b80_8208;
			AX = (product & 0xFFFF);      // low 16 bits
			DX = (product >> 16);          // high 16 bits

			int dividend = ((int)DX << 16) | (unsigned short)AX;
			short divisor = BP;

			AX = (short)(dividend / divisor);
			DX = (short)(dividend % divisor);

			AX += WORD_ARRAY_4b80_81d8[3];
			WORD_4b80_81b0 = AX;

			AX = WORD_ARRAY_4b80_81d8[16];
			AX -= WORD_ARRAY_4b80_81d8[4];

			product = AX * WORD_4b80_8208;
			AX = (product & 0xFFFF);      // low 16 bits
			DX = (product >> 16);          // high 16 bits

			dividend = ((int)DX << 16) | (unsigned short)AX;
			divisor = BP;

			AX = (short)(dividend / divisor);
			DX = (short)(dividend % divisor);

			AX += WORD_ARRAY_4b80_81d8[4];
			WORD_4b80_81b4 = AX;

			AX = WORD_ARRAY_4b80_81d8[17];
			AX -= WORD_ARRAY_4b80_81d8[5];

			product = AX * WORD_4b80_8208;
			AX = (product & 0xFFFF);      // low 16 bits
			DX = (product >> 16);          // high 16 bits

			dividend = ((int)DX << 16) | (unsigned short)AX;
			divisor = BP;

			AX += WORD_ARRAY_4b80_81d8[5];
			WORD_4b80_81b8 = AX;

			WORD_4b80_81ae = 0;
			WORD_4b80_81b2 = 0;
			WORD_4b80_81b6 = 0;

			AX = WORD_ARRAY_4b80_81d8[21];
			AX -= WORD_ARRAY_4b80_81d8[9];

			product = AX * WORD_4b80_8208;
			AX = (product & 0xFFFF);      // low 16 bits
			DX = (product >> 16);          // high 16 bits

			dividend = ((int)DX << 16) | (unsigned short)AX;
			divisor = BP;

			AX += WORD_ARRAY_4b80_81d8[9];
			WORD_4b80_81c6 = AX;

			AX = WORD_ARRAY_4b80_81d8[22];
			AX -= WORD_ARRAY_4b80_81d8[10];

			product = AX * WORD_4b80_8208;
			AX = (product & 0xFFFF);      // low 16 bits
			DX = (product >> 16);          // high 16 bits

			dividend = ((int)DX << 16) | (unsigned short)AX;
			divisor = BP;

			AX += WORD_ARRAY_4b80_81d8[10];
			WORD_4b80_81c8 = AX;

			AX = WORD_ARRAY_4b80_81d8[23];
			AX -= WORD_ARRAY_4b80_81d8[11];

			product = AX * WORD_4b80_8208;
			AX = (product & 0xFFFF);      // low 16 bits
			DX = (product >> 16);          // high 16 bits

			dividend = ((int)DX << 16) | (unsigned short)AX;
			divisor = BP;

			AX += WORD_ARRAY_4b80_81d8[11];
			WORD_4b80_81ca = AX;

			BP = 39;

			AX = WORD_4b80_81c6;
			AX -= WORD_4b80_81b0;
			EAX = AX << 16;

			int64_t dividend2 = (int64_t)(int32_t)EAX;
			int32_t divisor2 = (int32_t)BP;

			EAX = (int32_t)(dividend2 / divisor2);
			EDX = (int32_t)(dividend2 % divisor2);

			DWORD_VALUE3 = EAX;

			AX = WORD_4b80_81c8;
			AX -= WORD_4b80_81b4;
			EAX = AX << 16;

			dividend2 = (int64_t)(int32_t)EAX;
			divisor2 = (int32_t)BP;

			EAX = (int32_t)(dividend2 / divisor2);
			EDX = (int32_t)(dividend2 % divisor2);

			DWORD_VALUE4 = EAX;

			AX = WORD_4b80_81ca;
			AX -= WORD_4b80_81b8;
			EAX = AX << 16;

			dividend2 = (int64_t)(int32_t)EAX;
			divisor2 = (int32_t)BP;

			EAX = (int32_t)(dividend2 / divisor2);
			EDX = (int32_t)(dividend2 % divisor2);

			DWORD_VALUE5 = EAX;

			ECX = WORD_4b80_81b4 * WORD_4b80_81d4;
			DWORD_4b80_819e = ECX;

			EAX = WORD_4b80_81c8 * WORD_4b80_81d4;
			EAX -= ECX;

			EAXEDX = EAX * DWORD_4b80_81a6;

			EAX = (int)EAXEDX;
			EDX = EAXEDX >> 32;

			DWORD_VALUE1 = EAX;
			DWORD_VALUE2 = EDX;
			DWORD_4b80_81a2 = 0;

			loopCount = 40; // 0028 in hexadecimal

			do
			{
				EAX = DWORD_4b80_819a;
				EBP = DWORD_4b80_819e;
				if (EBP != 0)
				{

					dividend2 = (int64_t)(int32_t)EAX;
					divisor2 = (int32_t)EBP;

					EAX = (int32_t)(dividend2 / divisor2);
					EDX = (int32_t)(dividend2 % divisor2);

					if (EAX < 0)
					{
						EAXEDX = (long long)EAX * (long long)DWORD_4b80_81aa;
						EAX = EAXEDX;
						EDX = EAXEDX >> 32;
						EAX = ((unsigned int)EAX >> 31) | (EDX << 1);
					}

					EBX = EAX;
					EBP = ((WORD_4b80_81ae & 0xFFFF) | ((WORD_4b80_81b0 & 0xFFFF) << 16));
					EAXEDX = (long long)EAX * (long long)EBP;
					EAX = EAXEDX;
					EDX = EAXEDX >> 32;
					EAX = ((unsigned int)EAX >> 24) | (EDX << 8);
					EAX += fogState.PlayerX + fogState.WORD_4b80_191b;
					EAX = EAX >> 6;
					std::swap(EAX, EBX);

					EBP = ((WORD_4b80_81b6 & 0xFFFF) | ((WORD_4b80_81b8 & 0xFFFF) << 16));
					EAXEDX = (long long)EAX * (long long)EBP;
					EAX = EAXEDX;
					EDX = EAXEDX >> 32;
					EAX = ((unsigned int)EAX >> 24) | (EDX << 8);
					EAX += fogState.PlayerZ + fogState.WORD_4b80_191b;
					EAX = EAX >> 6;

					BX = EBX;
					BX &= 0x7F;
					BX <<= 7;

					AX = EAX;
					AX &= 0x7F;

					BX += AX;

					AX = fogState.fogTxt[BX];
				}
				else
				{
					AX = (EAX & 0x00FF) | 0x0C00;
				}

				// Write the value to the sample buffer FOGTXTSample, a short value array, at FOGTXTSampleIndex.
				g_fogTxtSamples[fogTxtSampleIndex] = AX; // Write the calculated value
				fogTxtSampleIndex++;

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
				WORD_4b80_81ae = sum;
				WORD_4b80_81b0 = sum >> 16;

				sum = (WORD_4b80_81b2 & 0xFFFF | ((WORD_4b80_81b4 & 0xFFFF) << 16));
				sum += DWORD_VALUE4;
				WORD_4b80_81b2 = sum;
				WORD_4b80_81b4 = sum >> 16;

				sum = (WORD_4b80_81b6 & 0xFFFF | ((WORD_4b80_81b8 & 0xFFFF) << 16));
				sum += DWORD_VALUE5;
				WORD_4b80_81b6 = sum;
				WORD_4b80_81b8 = sum >> 16;

				loopCount--;
			} while (loopCount != 0);

			WORD_4b80_8208++;
		} while (WORD_4b80_8208 != 25); // Rows
	}

	void ApplySampledFogData(Span<const uint8_t> fogLgt, Span<short> ESArray)
	{
		ESArray.fill(0x2566);

		constexpr short WORD_4b80_a76a = 0x533C; // This is variable, but in testing it was 0x533C, which matched the location (533C:0000) put in ES and represented here as  "ESArray". It might always be that when this function is called.
		constexpr short WORD_4b80_a784 = 0x92; // Variable, but might always be 0x92 when fog functions called

		auto ApplyNewData = [&]()
		{
			BX += DX;
			CX = (CX & 0xFF) | ((BX & 0xFF) << 8);
			BX = (BX & 0xFF00) | (ESArray[DI / 2] & 0xFF);
			AX = (AX & 0xFF00) | fogLgt[BX];
			BX = (CX & 0xFF00) >> 8;
			DX += BP;
			BX += DX;
			CX = (CX & 0xFF) | ((BX & 0xFF) << 8);
			BX = (BX & 0xFF00) | ((ESArray[DI / 2] & 0xFF00) >> 8);
			AX = (AX & 0xFF) | (fogLgt[BX] << 8);
			ESArray[DI / 2] = AX;
			DI += 2;
			BX = (CX & 0xFF00) >> 8;
			DX += BP;
		};

		auto IterateOverData = [&]()
		{
			ApplyNewData();
			ApplyNewData();
			ApplyNewData();
			ApplyNewData();
			SI++;
			SI++;
			DX = g_fogTxtSamples[SI / 2];
			BP = g_fogTxtSamples[(SI + 2) / 2];
			BP -= DX;
			BP >>= 3;
			g_fogTxtSamples[SI / 2] = DX + g_fogTxtSamples[(SI - 90) / 2];
		};

		// Aaron: is this zeroing the horizon line?
		for (int i = 0; i < 40; i++)
		{
			g_fogTxtSamples[405 + i] = 0;
		}

		g_fogTxtSamples[43] = WORD_4b80_a76a;
		g_fogTxtSamples[40] = (WORD_4b80_a784 + 7) >> 3;
		g_fogTxtSamples[41] = 170;
		g_fogTxtSamples[42] = 0;

		do
		{
			for (int i = 0; i < 40; i++)
			{
				g_fogTxtSamples[i] = (g_fogTxtSamples[85 + i] - g_fogTxtSamples[45 + i]) >> 3;
			}

			DI = g_fogTxtSamples[42];
			ES = g_fogTxtSamples[43]; // 0x533C in testing, used for location of ESArray
			CX = 8;

			if (g_fogTxtSamples[40] == 1)
			{
				CX -= 6;
			}

			do
			{
				SI = g_fogTxtSamples[41] - 80;
				BX = 0;
				DX = g_fogTxtSamples[SI / 2];
				BP = (g_fogTxtSamples[(SI + 2) / 2] - DX) >> 3;
				g_fogTxtSamples[SI / 2] = DX + g_fogTxtSamples[0];

				for (int i = 0; i < 39; i++)
				{
					IterateOverData();
				}

				ApplyNewData();
				SI++;
				CX--;
			} while (CX != 0);

			g_fogTxtSamples[42] = DI;
			g_fogTxtSamples[40]--;
		} while (g_fogTxtSamples[40] != 0);
	}
}

ArenaFogState::ArenaFogState()
{
	this->PlayerX = 0;
	this->PlayerZ = 0;
	this->PlayerAngle = 0;
	this->WORD_4b80_191b = 4;
	this->WORD_4b80_191d = 4;
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
	this->PlayerX = originalPlayerPos.x;
	this->PlayerZ = originalPlayerPos.y;

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

	this->PlayerAngle = getOriginalAngle(-playerDir.y, -playerDir.x);

	constexpr double FOG_SECONDS_PER_FRAME = 1.0 / 25.0;

	this->currentSeconds += dt;
	if (this->currentSeconds >= FOG_SECONDS_PER_FRAME)
	{
		this->currentSeconds = std::fmod(this->currentSeconds, FOG_SECONDS_PER_FRAME);
		this->WORD_4b80_191b += 4;
		this->WORD_4b80_191d += 4;
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
	std::copy(std::begin(exeDataWeather.fogTxtSampleHelper), std::end(exeDataWeather.fogTxtSampleHelper), std::begin(WORD_ARRAY_4b80_81d8));
	
	SampleFOGTXT(fogState, exeData);

	Span<short> ESArray = g_ESArray;
	ApplySampledFogData(fogState.fogLgt, ESArray);

	outPixels.fill(0);

	for (int y = 0; y < ESHeight; y++)
	{
		for (int x = 0; x < ESWidth; x++)
		{
			const int srcIndex = x + (y * ESWidth);
			const uint16_t sample = ESArray[srcIndex];
			const uint8_t lightLevel = static_cast<uint8_t>(sample >> 8);
			const uint8_t dither = static_cast<uint8_t>(sample);

			const bool shouldDither = dither != 0;
			const uint8_t calculatedLightLevel = lightLevel + (shouldDither ? 1 : 0);
			outPixels.set(x, y, calculatedLightLevel);
		}
	}

	/*for (int row = 0; row < fogRows; row++)
	{
		for (int column = 0; column < fogColumns; column++)
		{
			const int srcIndex = column + (row * fogColumns) + fogTxtSampleExtraCount;
			const uint16_t sample = fogTxtSamples[srcIndex];
			const uint8_t lightLevel = static_cast<uint8_t>(sample >> 8);
			const uint8_t dither = static_cast<uint8_t>(sample);

			constexpr int tileDimension = 8;
			for (int j = 0; j < tileDimension; j++)
			{
				for (int i = 0; i < tileDimension; i++)
				{
					const int x = (column * tileDimension) + i;
					const int y = (row * tileDimension) + j;

					const bool shouldDither = (dither & (1 << i)) != 0;
					const uint8_t calculatedLightLevel = lightLevel + (shouldDither ? 1 : 0);
					outPixels.set(x, y, calculatedLightLevel);
				}
			}
		}
	}*/
}
