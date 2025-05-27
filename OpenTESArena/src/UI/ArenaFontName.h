#ifndef ARENA_FONT_NAME_H
#define ARENA_FONT_NAME_H

namespace ArenaFontName
{
	// Can't be std::string due to global initialization order issues.
	constexpr const char *A = "FONT_A.DAT";
	constexpr const char *Arena = "ARENAFNT.DAT";
	constexpr const char *B = "FONT_B.DAT";
	constexpr const char *C = "FONT_C.DAT";
	constexpr const char *Char = "CHARFNT.DAT";
	constexpr const char *D = "FONT_D.DAT";
	constexpr const char *Four = "FONT4.DAT";
	constexpr const char *S = "FONT_S.DAT";
	constexpr const char *Teeny = "TEENYFNT.DAT";

	constexpr const char *FontPtrs[] =
	{
		ArenaFontName::A,
		ArenaFontName::Arena,
		ArenaFontName::B,
		ArenaFontName::C,
		ArenaFontName::Char,
		ArenaFontName::D,
		ArenaFontName::Four,
		ArenaFontName::S,
		ArenaFontName::Teeny
	};
}

#endif
