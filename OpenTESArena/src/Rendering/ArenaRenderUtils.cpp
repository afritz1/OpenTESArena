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
	void FogRotateVector(int angle, short &x, short &y, const ExeDataWeather &exeDataWeather)
	{
		const Span<const int16_t> cosineTable = exeDataWeather.fogCosineTable;
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

	void SampleFOGTXT(const ArenaFogState &fogState, Span<uint16_t> fogTxtSamples, const ExeDataWeather &exeDataWeather)
	{
		short WORD_ARRAY_4b80_81d8[24]; // @47708. Both read from and written to.
		std::copy(std::begin(exeDataWeather.fogTxtSampleHelper), std::end(exeDataWeather.fogTxtSampleHelper), std::begin(WORD_ARRAY_4b80_81d8));

		int DWORD_4b80_819e = 0;
		int DWORD_4b80_81a2 = 0;
		constexpr int DWORD_4b80_81a6 = 0x6906904; // Constant value. @476D6.

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

		int intValue = 0; // Used here for 32-bit operations
		int intValue2 = 0; // Used here for 32-bit operations  @todo this was uninitialized in Allofich's code
		int intValue3 = 0; // Used here for 32-bit operations
		long long longValue = 0; // Used here for 64-bit operations

		int loopCount = 4;
		int index = 0;

		do
		{
			AX = WORD_ARRAY_4b80_81d8[index];
			CX = WORD_ARRAY_4b80_81d8[index + 2];
			DI = 511;
			DI -= fogState.PlayerAngle; // PlayerAngle is never greater than 511

			FogRotateVector(DI, AX, CX, exeDataWeather);

			BX = WORD_ARRAY_4b80_81d8[index + 1];
			WORD_ARRAY_4b80_81d8[index + 3] = AX;

			WORD_ARRAY_4b80_81d8[index + 4] = BX;
			WORD_ARRAY_4b80_81d8[index + 5] = CX;
			index += 6;
			loopCount--;
		} while (loopCount != 0);

		constexpr int DWORD_4b80_819a = 0xD0300000; // Original game does a few calculations here to get the value, but it will always be this result
		constexpr int DWORD_4b80_81aa = 0xDD5D5D5E; // Original game does a few calculations here to get the value, but it will always be this result
		WORD_4b80_8208 = 0;

		int fogTxtSampleIndex = 0;

		do
		{
			BP = WORD_4b80_a784;
			BP >>= 3;

			AX = WORD_ARRAY_4b80_81d8[15];
			AX -= WORD_ARRAY_4b80_81d8[3];

			intValue = AX * WORD_4b80_8208;
			AX = intValue;
			DX = intValue >> 16;

			intValue = AX / BP;
			DX = intValue >> 16;

			AX += WORD_ARRAY_4b80_81d8[3];
			WORD_4b80_81b0 = AX;

			AX = WORD_ARRAY_4b80_81d8[16];
			AX -= WORD_ARRAY_4b80_81d8[4];

			intValue = AX * WORD_4b80_8208;
			AX = intValue;
			DX = intValue >> 16;

			intValue = AX / BP;
			DX = intValue >> 16;

			AX += WORD_ARRAY_4b80_81d8[4];
			WORD_4b80_81b4 = AX;

			AX = WORD_ARRAY_4b80_81d8[17];
			AX -= WORD_ARRAY_4b80_81d8[5];

			intValue = AX * WORD_4b80_8208;
			AX = intValue;
			DX = intValue >> 16;

			AX += WORD_ARRAY_4b80_81d8[5];
			WORD_4b80_81b8 = AX;

			WORD_4b80_81ae = 0;
			WORD_4b80_81b2 = 0;
			WORD_4b80_81b6 = 0;

			AX = WORD_ARRAY_4b80_81d8[21];
			AX -= WORD_ARRAY_4b80_81d8[9];

			intValue = AX * WORD_4b80_8208;
			AX = intValue;

			AX += WORD_ARRAY_4b80_81d8[9];
			WORD_4b80_81c6 = AX;

			AX = WORD_ARRAY_4b80_81d8[22];
			AX -= WORD_ARRAY_4b80_81d8[10];

			intValue = AX * WORD_4b80_8208;
			AX = intValue;

			AX += WORD_ARRAY_4b80_81d8[10];
			WORD_4b80_81c8 = AX;

			AX = WORD_ARRAY_4b80_81d8[23];
			AX -= WORD_ARRAY_4b80_81d8[11];

			intValue = AX * WORD_4b80_8208;
			AX = intValue;

			AX += WORD_ARRAY_4b80_81d8[11];
			WORD_4b80_81ca = AX;

			BP = 39;

			AX = WORD_4b80_81c6;
			AX -= WORD_4b80_81b0;
			longValue = static_cast<long long>(AX) << 16;
			longValue /= BP;
			DWORD_VALUE3 = longValue;

			AX = WORD_4b80_81c8;
			AX -= WORD_4b80_81b4;
			longValue = static_cast<long long>(AX) << 16;
			longValue /= BP;
			DWORD_VALUE4 = longValue;

			AX = WORD_4b80_81ca;
			AX -= WORD_4b80_81b8;
			longValue = static_cast<long long>(AX) << 16;
			longValue /= BP;
			DWORD_VALUE5 = longValue;

			intValue = WORD_4b80_81b4 * WORD_4b80_81d4;

			DWORD_4b80_819e = intValue2;

			AX = WORD_4b80_81c8;
			intValue = AX * WORD_4b80_81d4;
			intValue2 -= intValue;

			longValue = intValue2 * DWORD_4b80_81a6;

			intValue = longValue;
			intValue2 = longValue >> 32;

			DWORD_VALUE1 = intValue;
			DWORD_VALUE2 = intValue2;
			DWORD_4b80_81a2 = 0;

			loopCount = 40; // Columns

			do
			{
				longValue = DWORD_4b80_819a;
				intValue = DWORD_4b80_819e;
				if (intValue != 0)
				{
					longValue /= intValue;
					intValue = longValue;
					if (intValue < 0)
					{
						// untested
						intValue *= DWORD_4b80_81aa;
						intValue2 = ((unsigned int)intValue2 >> 31) | ((unsigned int)intValue << 1);
					}

					intValue3 = intValue; // Store for using below

					longValue = intValue * (long long)intValue * (long long)((unsigned short)WORD_4b80_81ae | ((unsigned short)WORD_4b80_81b0 << 16));
					intValue = longValue;
					intValue2 = longValue >> 32;
					intValue = ((unsigned int)intValue >> 24) | ((unsigned int)intValue2 << 8);
					intValue += fogState.PlayerX + fogState.WORD_4b80_191b;
					intValue = (unsigned int)intValue >> 6;

					longValue = (long long)intValue3 * (long long)((unsigned short)WORD_4b80_81b6 | ((unsigned short)WORD_4b80_81b8 << 16));
					intValue2 = longValue;
					intValue3 = longValue >> 32;
					intValue2 = ((unsigned int)intValue2 >> 24) | ((unsigned int)intValue3 << 8);

					intValue2 += fogState.PlayerY + fogState.WORD_4b80_191d;
					intValue2 = (unsigned int)intValue2 >> 6;

					intValue &= 0x7f;
					intValue <<= 7;

					intValue2 &= 0x7f;

					intValue += intValue2;
					//intValue <<= 1; // Convert texel index to byte index.

					AX = fogState.fogTxt[intValue]; // Get 2-byte value from data read from FOG.TXT
				}
				else
				{
					// untested
					AX = 0x0C00;
				}

				//Store the value to the sample buffer and move the pointer to the buffer forward 2 bytes to prepare for the next iteration
				fogTxtSamples[fogTxtSampleIndex] = AX;
				fogTxtSampleIndex++;

				//Get CF for ADC instruction
				bool carry = false;
				uint32_t temp = static_cast<uint32_t>(DWORD_4b80_81a2) + static_cast<uint32_t>(DWORD_VALUE1);
				if (temp < DWORD_4b80_81a2)
				{
					carry = true; // overflow occurred
				}

				DWORD_4b80_81a2 = temp;   // Store the result back

				DWORD_4b80_819e += DWORD_VALUE2;
				if (carry)
				{
					DWORD_4b80_819e++;  // ADC instruction, so add carry flag
				}

				WORD_4b80_81ae += (DWORD_VALUE3 & 0x0000FFFF);
				WORD_4b80_81b0 += (DWORD_VALUE3 & 0xFFFF0000) >> 16;
				WORD_4b80_81b2 += (DWORD_VALUE4 & 0x0000FFFF);
				WORD_4b80_81b4 += (DWORD_VALUE4 & 0xFFFF0000) >> 16;
				WORD_4b80_81b6 += (DWORD_VALUE5 & 0x0000FFFF);
				WORD_4b80_81b8 += (DWORD_VALUE5 & 0xFFFF0000) >> 16;

				loopCount--;
			} while (loopCount != 0);

			WORD_4b80_8208++;
		} while (WORD_4b80_8208 != 25); // Rows
	}

	void ApplySampledFogData(Span<uint16_t> fogTxtSamples, Span<const uint8_t> fogLgt)
	{
		constexpr short WORD_4b80_81ae = 0x533C; // This is variable, but in testing it was 0x533C, which matched the location (533C:0000) put in ES and represented here as  "ESArray". It might always be that when this function is called.
		constexpr short WORD_4b80_a784 = 0x92; // Variable, but might always be 0x92 when fog functions called
		
		uint8_t ESArr[320]; // Unknown, but presumably for the 320 columns of pixels on the screen
		std::fill(std::begin(ESArr), std::end(ESArr), 0);
		Span<uint8_t> ESArray = ESArr;

		short AX = 0;
		short BX = 0;
		short CX = 0;
		short DI = 0;
		short DX = 0;
		short BP = 0;
		short SI = 0;
		short DS = 0;
		short ES = 0;

		auto ApplyNewData = [&]()
		{
			BX += DX;
			CX = (CX & 0xFF) | ((BX & 0xFF) << 8);
			BX = (BX & 0xFF00) | ESArray[DI];
			AX = fogLgt[BX];
			BX = (CX & 0xFF00) >> 8;
			DX += BP;
			BX += DX;
			CX = (CX & 0xFF) | ((BX & 0xFF) << 8);
			BX = (BX & 0xFF00) | ESArray[DI + 1];
			AX = (AX & 0xFF) | (fogLgt[BX] << 8);
			ESArray[DI] = (AX >> 8) & 0xFF;
			ESArray[DI + 1] = AX & 0xFF;
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
			DX = fogTxtSamples[SI];
			BP = fogTxtSamples[SI + 1];
			BP -= DX;
			BP >>= 3;
			fogTxtSamples[SI] = DX + fogTxtSamples[SI - 45];
		};

		// Aaron: is this zeroing the horizon line?
		for (int i = 0; i < 40; i++)
		{
			fogTxtSamples[405 + i] = 0;
		}

		fogTxtSamples[28] = WORD_4b80_81ae;
		fogTxtSamples[25] = (WORD_4b80_a784 + 7) >> 3;
		fogTxtSamples[26] = 170;
		fogTxtSamples[27] = 0;

		do
		{
			for (int i = 0; i < 40; i++)
			{
				fogTxtSamples[i] = (fogTxtSamples[85 + i] - fogTxtSamples[45 + i]) >> 3;
			}

			DS = fogTxtSamples[42];
			ES = fogTxtSamples[43]; // 0x533C in testing, used for location of ESArray
			CX = 8;

			if (fogTxtSamples[40] == 1)
			{
				CX -= 6;
			}

			do
			{
				SI = fogTxtSamples[41] - 80;
				DI = 0;
				DX = fogTxtSamples[SI / 2];
				BP = (fogTxtSamples[(SI + 2) / 2] - DX) >> 3;
				fogTxtSamples[SI / 2] = DX + fogTxtSamples[0];

				for (int i = 0; i < 39; i++)
				{
					IterateOverData();
				}

				ApplyNewData();
				SI++;
				CX--;
			} while (CX != 0);

			fogTxtSamples[42] = DI;
			fogTxtSamples[40]--;
		} while (fogTxtSamples[40] != 0);
	}
}

ArenaFogState::ArenaFogState()
{
	this->PlayerX = 0;
	this->PlayerY = 0;
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
	const OriginalInt2 originalPlayerPos = GameWorldUiModel::getOriginalPlayerPosition(playerPos, mapType);
	this->PlayerX = originalPlayerPos.x;
	this->PlayerY = originalPlayerPos.y;

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

	constexpr int fogColumns = 40;
	constexpr int fogRows = 25;
	uint16_t fogTxtSamples[fogColumns * fogRows];
	std::fill(std::begin(fogTxtSamples), std::end(fogTxtSamples), 0);

	SampleFOGTXT(fogState, fogTxtSamples, exeData.weather);
	ApplySampledFogData(fogTxtSamples, fogState.fogLgt);

	for (int y = 0; y < 200; y++)
	{
		for (int x = 0; x < 320; x++)
		{
			const int srcIndex = (x / 8) + ((y / 8) * 40);
			//const int dstIndex = x + (y * 320);
			const uint16_t srcPixel = fogTxtSamples[srcIndex];
			const uint8_t dstPixel = static_cast<uint8_t>(srcPixel);
			outPixels.set(x, y, dstPixel);
		}
	}
}
