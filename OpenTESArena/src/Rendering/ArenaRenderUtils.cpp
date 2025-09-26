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

	void SampleFOGTXT(const ArenaFogState &fogState, Span<uint16_t> fogTxtSamples, const ExeData &exeData)
	{
		const ExeDataWeather &exeDataWeather = exeData.weather;
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

		int EAX = 0;
		int EBX = 0;
		int ECX = 0;
		int EDX = 0;
		int EBP = 0;
		int64_t EAXEDX = 0;

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

		constexpr int DWORD_4b80_819a = 0xD0300000; // Original game does a few calculations here to get the value, but it will always be this result
		constexpr int DWORD_4b80_81aa = 0xDD5D5D5E; // Original game does a few calculations here to get the value, but it will always be this result
		WORD_4b80_8208 = 0;

		int fogTxtSampleIndex = 0;

		do
		{
			BP = WORD_4b80_a784; // Seems to always be 0092 when this function is called
			BP >>= 3; // 0012

			AX = WORD_ARRAY_4b80_81d8[15]; // 03F8
			AX -= WORD_ARRAY_4b80_81d8[3]; // Sub 03F8, get 0
			AX *= WORD_4b80_8208; // 0
			AX /= BP; // 0    
			AX += WORD_ARRAY_4b80_81d8[3]; // Add 03F8, get 03F8
			WORD_4b80_81b0 = AX; // 03F8

			AX = WORD_ARRAY_4b80_81d8[16]; // FBFA
			AX -= WORD_ARRAY_4b80_81d8[4]; // Sub 0406, get F7F4
			AX *= WORD_4b80_8208; // 0    
			AX /= BP; // 0   
			AX += WORD_ARRAY_4b80_81d8[4]; // Add 0406, get 0406
			WORD_4b80_81b4 = AX; // 0406

			AX = WORD_ARRAY_4b80_81d8[17]; // FC0B
			AX -= WORD_ARRAY_4b80_81d8[5]; // Sub FC0B, get 0 
			AX *= WORD_4b80_8208; // 0
			AX /= BP; // 0 
			AX += WORD_ARRAY_4b80_81d8[5]; // Add FC0B, get FC0B
			WORD_4b80_81b8 = AX; // FC0B

			WORD_4b80_81ae = 0;
			WORD_4b80_81b2 = 0;
			WORD_4b80_81b6 = 0;

			AX = WORD_ARRAY_4b80_81d8[21]; // FBED
			AX -= WORD_ARRAY_4b80_81d8[9]; // Sub FBED, get 0
			AX *= WORD_4b80_8208; // 0
			AX /= BP; // 0
			AX += WORD_ARRAY_4b80_81d8[9]; // Add FBED, get FBED
			WORD_4b80_81c6 = AX; // FBED

			AX = WORD_ARRAY_4b80_81d8[22]; // FBFA
			AX -= WORD_ARRAY_4b80_81d8[10]; // Sub 0406, get F7F4  
			AX *= WORD_4b80_8208; // 0
			AX /= BP; // 0 
			AX += WORD_ARRAY_4b80_81d8[10]; // Add 0406, get 0406
			WORD_4b80_81c8 = AX; // 0406

			AX = WORD_ARRAY_4b80_81d8[23]; // FC24
			AX -= WORD_ARRAY_4b80_81d8[11]; // Sub FC24, get 0
			AX *= WORD_4b80_8208; // 0
			AX /= BP; // 0
			AX += WORD_ARRAY_4b80_81d8[11]; // Add FC24, get FC24
			WORD_4b80_81ca = AX; // FC24

			BP = 39; // 0027 (hexadecimal)

			AX = WORD_4b80_81c6; // FBED
			AX -= WORD_4b80_81b0; // Sub 03F8, get F7F5
			EAX = AX << 16; // F7F50000
			EAX /= BP; // Div by 0027, get FFCB3484
			DWORD_VALUE3 = EAX; // FFCB3484

			AX = WORD_4b80_81c8; // 0406
			AX -= WORD_4b80_81b4; // Sub 0406, get 0
			EAX = AX << 16; // 0
			EAX /= BP; // 0
			DWORD_VALUE4 = EAX; // 0

			AX = WORD_4b80_81ca; // FC24
			AX -= WORD_4b80_81b8; // Sub FC0B, get 0019
			EAX = AX << 16; // 00190000
			EAX /= BP; // Div by 0027, get 0000A41A
			DWORD_VALUE5 = EAX; // 0000A41A

			ECX = WORD_4b80_81b4 * WORD_4b80_81d4; // 0406 * FC00, get FFEFE800
			DWORD_4b80_819e = ECX; // FFEFE800

			EAX = WORD_4b80_81c8 * WORD_4b80_81d4; // 0406 * FC00, get FFEFE800
			EAX -= ECX; // Sub FFEFE800 from FFEFE800, get 0

			EAXEDX = EAX * DWORD_4b80_81a6; // Mul by constant value 6906904, get 0

			EAX = (int)EAXEDX; // 0
			EDX = EAXEDX >> 32; // 0

			DWORD_VALUE1 = EAX; // 0
			DWORD_VALUE2 = EDX; // 0
			DWORD_4b80_81a2 = 0;

			loopCount = 40; // 0028 in hexadecimal

			do
			{
				EAX = DWORD_4b80_819a; // D0300000
				EBP = DWORD_4b80_819e; // FFEFE800
				if (EBP != 0)
				{ // true
					EAX /= EBP; // Div by FFEFE800, get 000002F8
					if (EAX < 0)
					{ // false
						EAXEDX = (long long)EAX * (long long)DWORD_4b80_81aa;
						EAX = EAXEDX;
						EDX = EAXEDX >> 32;
						EAX = ((unsigned int)EAX >> 31) | (EDX << 1);
					}

					EBX = EAX; // 000002F8
					EBP = (WORD_4b80_81ae | (WORD_4b80_81b0 << 16)); // WORD_4b80_81ae is 0000, WORD_4b80_81b0 is 03F8. Combine and get 03F80000
					EAXEDX = (long long)EAX * (long long)EBP; // Mul 000002F8 by 03F80000, get 0000000BC8400000
					EAX = EAXEDX; // C8400000
					EDX = EAXEDX >> 32; // 0000000B
					EAX = ((unsigned int)EAX >> 24) | (EDX << 8); // 00000BC8
					EAX += fogState.PlayerX + fogState.WORD_4b80_191b; // PlayerX is 3205 for my test case. WORD_4b80_191b is 0004 on first call to fog function, +4 for every subsequent call to it. 00000BC8 + 3205 + 0004 to get 00003DD1
					EAX = EAX >> 6; // 000000F7
					std::swap(EAX, EBX); // EAX = 000002F8, EBX = 000000F7

					EBP = (WORD_4b80_81b6 | (WORD_4b80_81b8 << 16)); // WORD_4b80_81b6 is 0000, WORD_4b80_81b8 is FC0B. Combine and get FC0B0000
					EAXEDX = (long long)EAX * (long long)EBP; // Mul 000002F8 by FC0B0000, get FFFFFFF440A80000
					EAX = EAXEDX; // 40A80000
					EDX = EAXEDX >> 32; // FFFFFFF4
					EAX = ((unsigned int)EAX >> 24) | (EDX << 8); // FFFFF440        
					EAX += fogState.PlayerZ + fogState.WORD_4b80_191d; // PlayerZ is 01E7 for my test case. WORD_4b80_191d is 0004 on first call to fog function, +4 for every subsequent call to it, the same as WORD_4b80_191b. FFFFF440 + 01E7 + 0004 to get FFFFF62B
					EAX = EAX >> 6; // FFFFFFD8

					BX = EBX; // 00F7
					BX &= 0x7F; // 0077
					BX <<= 7; // 3B80

					AX = EAX; // FFD8
					AX &= 0x7F; // 0058

					BX += AX; // 3BD8
					//BX <<= 1; // 77B0 // Aaron: don't convert texel index to byte index.

					AX = fogState.fogTxt[BX]; // 05D9
				}
				else
				{
					AX = (AX & 0x00FF) | 0x0C00;
				}

				// Write the value to the sample buffer FOGTXTSample, a short value array, at FOGTXTSampleIndex, which should initialized to 45 (decimal) at the start of SampleFOGTXT.
				fogTxtSamples[fogTxtSampleIndex] = AX; // Write the calculated value 05D9
				fogTxtSampleIndex++;

				// Next is, in assembly:
				// ADD dword[81a2], DWORD_VALUE1
				// ADC dword[819e], DWORD_VALUE2
				// ADD dword[81ae], DWORD_VALUE3
				// ADD dword[81b2], DWORD_VALUE4
				// ADD dword[81b6], DWORD_VALUE5

				// The ADC instruction adds any carry from the preceding ADD instruction, so we need to get the carry
				bool carry = false;
				uint32_t before = DWORD_4b80_81a2; // 00000000
				DWORD_4b80_81a2 += DWORD_VALUE1; // Add 0, get 0
				if (DWORD_4b80_81a2 < before)
				{ // false
					carry = true; // overflow occurred
				}

				DWORD_4b80_819e += DWORD_VALUE2; // DWORD_4b80_819e is FFEFE800, add 0
				if (carry)
				{ // false
					DWORD_4b80_819e++;
				}

				// WORD_4b80_81ae is 0000. WORD_4b80_81b0 is 03F8. DWORD_VALUE3 is FFCB3484.
				// WORD_4b80_81ae becomes 3484. WORD_4b80_81b0 becomes 03C3.
				WORD_4b80_81ae += (DWORD_VALUE3 & 0x0000FFFF);
				WORD_4b80_81b0 += (DWORD_VALUE3 & 0xFFFF0000) >> 16;

				// WORD_4b80_81b2 is 0000. WORD_4b80_81b4 is 0406. DWORD_VALUE4 is 00000000.
				// WORD_4b80_81b2 and WORD_4b80_81b4 are unchanged.
				WORD_4b80_81b2 += (DWORD_VALUE4 & 0x0000FFFF);
				WORD_4b80_81b4 += (DWORD_VALUE4 & 0xFFFF0000) >> 16;

				// WORD_4b80_81b6 is 0000. WORD_4b80_81b8 is FC0B. DWORD_VALUE5 is 0000A41A.
				// WORD_4b80_81b6 becomes A41A. WORD_4b80_81b8 remains FC0B.
				WORD_4b80_81b6 += (DWORD_VALUE5 & 0x0000FFFF);
				WORD_4b80_81b8 += (DWORD_VALUE5 & 0xFFFF0000) >> 16;

				loopCount--;
			} while (loopCount != 0);

			WORD_4b80_8208++;
		} while (WORD_4b80_8208 != 25); // Rows
	}

	constexpr int ESWidth = ArenaRenderUtils::SCREEN_WIDTH;
	constexpr int ESHeight = ArenaRenderUtils::SCENE_VIEW_HEIGHT - 1;
	constexpr int ESElementCount = (ESWidth * ESHeight) / 2;
	short g_ESArray[ESElementCount]; // For 320 columns x 146 rows of screen pixels (moved here to avoid stack warning).

	void ApplySampledFogData(Span<uint16_t> fogTxtSamples, Span<const uint8_t> fogLgt)
	{
		constexpr short WORD_4b80_81ae = 0x533C; // This is variable, but in testing it was 0x533C, which matched the location (533C:0000) put in ES and represented here as  "ESArray". It might always be that when this function is called.
		constexpr short WORD_4b80_a784 = 0x92; // Variable, but might always be 0x92 when fog functions called

		Span<short> ESArray = g_ESArray;
		ESArray.fill(0);

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
			BX = (BX & 0xFF00) | (ESArray[DI / 2] & 0xFF);
			AX = (AX & 0xFF00) | fogLgt[BX];
			BX = (CX & 0xFF00) >> 8;
			DX += BP;
			BX += DX;
			CX = (CX & 0xFF) | ((BX & 0xFF) << 8);
			BX = (BX & 0xFF00) | (ESArray[DI / 2] & 0xFF00);
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
			DX = fogTxtSamples[SI / 2];
			BP = fogTxtSamples[(SI + 2) / 2];
			BP -= DX;
			BP >>= 3;
			fogTxtSamples[SI / 2] = DX + fogTxtSamples[(SI - 90) / 2];
		};

		// Aaron: is this zeroing the horizon line?
		for (int i = 0; i < 40; i++)
		{
			fogTxtSamples[405 + i] = 0;
		}

		fogTxtSamples[43] = WORD_4b80_81ae;
		fogTxtSamples[40] = (WORD_4b80_a784 + 7) >> 3;
		fogTxtSamples[41] = 170;
		fogTxtSamples[42] = 0;

		do
		{
			for (int i = 0; i < 40; i++)
			{
				fogTxtSamples[i] = (fogTxtSamples[85 + i] - fogTxtSamples[45 + i]) >> 3;
			}

			DI = fogTxtSamples[42];
			ES = fogTxtSamples[43]; // 0x533C in testing, used for location of ESArray
			CX = 8;

			if (fogTxtSamples[40] == 1)
			{
				CX -= 6;
			}

			do
			{
				SI = fogTxtSamples[41] - 80;
				BX = 0;
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
	const OriginalInt2 originalPlayerPos = GameWorldUiModel::getOriginalPlayerPosition(playerPos, mapType);
	this->PlayerX = originalPlayerPos.x * ArenaUnitsInteger;
	this->PlayerZ = originalPlayerPos.y * ArenaUnitsInteger;

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
	constexpr int fogTxtSampleExtraCount = 45;
	constexpr int fogTxtSampleCount = (fogColumns * fogRows) + fogTxtSampleExtraCount;
	uint16_t fogTxtSamples[fogTxtSampleCount];
	std::fill(std::begin(fogTxtSamples), std::end(fogTxtSamples), 0);

	Span<uint16_t> fogTxtSampleRange(fogTxtSamples + fogTxtSampleExtraCount, fogColumns * fogRows);
	SampleFOGTXT(fogState, fogTxtSampleRange, exeData);

	ApplySampledFogData(fogTxtSamples, fogState.fogLgt);

	for (int row = 0; row < fogRows; row++)
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
	}
}
