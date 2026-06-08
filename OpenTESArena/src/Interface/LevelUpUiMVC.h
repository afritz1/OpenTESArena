#pragma once

#include <string>

#include "../Assets/TextureUtils.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../UI/ArenaFontName.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextRenderUtils.h"
#include "../Utilities/Color.h"

#include "components/utilities/Buffer.h"

class Game;

struct InputActionCallbackValues;

namespace LevelUpUiModel
{
	std::string getPlayerHealth(Game &game);
	std::string getPlayerStamina(Game &game);
	std::string getPlayerSpellPoints(Game &game);

	std::string getBonusPointsRemainingText(Game &game);
}

namespace LevelUpUiView
{
	const Int2 AppearanceTextCenterPoint(
		(ArenaRenderUtils::SCREEN_WIDTH / 2) - 1,
		(ArenaRenderUtils::SCREEN_HEIGHT / 2) - 1);
	const std::string AppearanceTextFontName = ArenaFontName::Arena;
	const Color AppearanceTextColor(199, 199, 199);
	constexpr TextAlignment AppearanceTextAlignment = TextAlignment::MiddleCenter;
	constexpr int AppearanceTextLineSpacing = 1;
	constexpr UiTexturePatternType AppearanceTextPatternType = UiTexturePatternType::Dark;

	constexpr Rect AttributeButtonFirstRect(10, 52, 26, 8);
	constexpr Int2 UpDownButtonFirstTopLeftPosition(38, 48);

	constexpr Int2 BonusPointsTextureTopLeftPosition(45, 109);
	constexpr Int2 BonusPointsTextBoxTopLeftPosition(92, 113);
	const std::string BonusPointsFontName = ArenaFontName::Arena;
	constexpr Color BonusPointsTextColor(199, 199, 199);

	int getRemainingPointsTextBoxTextureWidth(int textWidth);
	int getRemainingPointsTextBoxTextureHeight(int textHeight);
}
