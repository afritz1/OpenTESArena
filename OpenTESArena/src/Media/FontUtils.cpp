#include <algorithm>
#include <array>

#include "FontUtils.h"
#include "FontName.h"

#include "components/debug/Debug.h"

namespace
{
	constexpr std::array<std::pair<FontName, const char*>, 9> FontNames =
	{
		{
			{ FontName::A, "FONT_A.DAT" },
			{ FontName::Arena, "ARENAFNT.DAT" },
			{ FontName::B, "FONT_B.DAT" },
			{ FontName::C, "FONT_C.DAT" },
			{ FontName::Char, "CHARFNT.DAT" },
			{ FontName::D, "FONT_D.DAT" },
			{ FontName::Four, "FONT4.DAT" },
			{ FontName::S, "FONT_S.DAT" },
			{ FontName::Teeny, "TEENYFNT.DAT" }
		}
	};
}

int FontUtils::getFontNameCount()
{
	return static_cast<int>(FontNames.size());
}

FontName FontUtils::getFontName(int index)
{
	DebugAssertIndex(FontNames, index);
	return FontNames[index].first;
}

const char *FontUtils::fromName(FontName name)
{
	const auto iter = std::find_if(FontNames.begin(), FontNames.end(),
		[name](const auto &pair)
	{
		return pair.first == name;
	});

	return (iter != FontNames.end()) ? iter->second : nullptr;
}
