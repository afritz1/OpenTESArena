#ifndef FONT_UTILS_H
#define FONT_UTILS_H

enum class FontName;

namespace FontUtils
{
	int getFontNameCount();
	FontName getFontName(int index);
	const char *fromName(FontName name);
}

#endif
