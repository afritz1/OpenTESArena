#ifndef EXE_TYPES_H
#define EXE_TYPES_H

#include <cstddef>
#include <cstdint>

// Various helper records for composite data in the executable, primarily used with the
// ExeData class.
namespace ExeTypes
{
	struct Rect16
	{
		static constexpr size_t SIZE = sizeof(int16_t) * 4;

		int16_t x, y, w, h;

		void init(const char *data);
	};

	// List box definition with buttons, scroll bar, and flags for alignment.
	struct List
	{
		static constexpr uint16_t ALIGNMENT_MASK = 0x3000;
		static constexpr uint16_t LEFT_ALIGNMENT = 0x0000;
		static constexpr uint16_t RIGHT_ALIGNMENT = 0x1000;
		static constexpr uint16_t CENTER_ALIGNMENT = 0x2000;

		Rect16 buttonUp, buttonDown, scrollBar, area;
		uint16_t flags;

		void init(const char *data);
	};
}

#endif
