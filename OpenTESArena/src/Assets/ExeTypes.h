#ifndef EXE_TYPES_H
#define EXE_TYPES_H

#include <cstdint>

// Various helper records for composite data in the executable, primarily used with the
// ExeData class.
class ExeTypes
{
private:
	ExeTypes() = delete;
	~ExeTypes() = delete;
public:
	struct Rect16
	{
		static const size_t SIZE;

		int16_t x, y, w, h;

		void init(const char *data);
	};

	// List box definition with buttons, scroll bar, and flags for alignment.
	struct List
	{
		static const uint16_t ALIGNMENT_MASK;
		static const uint16_t LEFT_ALIGNMENT;
		static const uint16_t RIGHT_ALIGNMENT;
		static const uint16_t CENTER_ALIGNMENT;

		Rect16 buttonUp, buttonDown, scrollBar, area;
		uint16_t flags;

		void init(const char *data);
	};
};

#endif
