#pragma once

#include "components/utilities/Enum.h"

enum class MouseButtonType
{
	Left = 1 << 0,
	Right = 1 << 1
};

AllowEnumFlags(MouseButtonType);
using MouseButtonTypeFlags = EnumFlags<MouseButtonType>;

enum class MouseWheelScrollType
{
	Down,
	Up
};
