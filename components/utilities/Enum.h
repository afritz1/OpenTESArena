#ifndef ENUM_H
#define ENUM_H

#include <type_traits>

template<typename EnumType>
class EnumFlags
{
private:
	using IntegralType = std::underlying_type_t<EnumType>;

	static_assert(std::is_enum_v<EnumType>);
	static_assert(std::is_unsigned_v<IntegralType>);
	// Wish there was a enum_values_all_one_bit_t<...>

	IntegralType value;
public:
	constexpr EnumFlags()
	{
		this->value = 0;
	}

	constexpr EnumFlags(EnumType e)
		: value(static_cast<IntegralType>(e))
	{
	}

	constexpr explicit EnumFlags(IntegralType integerValue)
		: value(integerValue)
	{
	}

	constexpr bool operator==(EnumFlags other) const
	{
		return this->value == other.value;
	}

	constexpr bool operator!=(EnumFlags other) const
	{
		return this->value != other.value;
	}

	constexpr EnumFlags operator|(EnumFlags other) const
	{
		return EnumFlags(this->value | other.value);
	}

	constexpr EnumFlags& operator|=(EnumFlags other)
	{
		this->value |= other.value;
		return *this;
	}

	constexpr EnumFlags operator&(EnumFlags other) const
	{
		return EnumFlags(this->value & other.value);
	}

	constexpr EnumFlags& operator&=(EnumFlags other)
	{
		this->value &= other.value;
		return *this;
	}

	constexpr EnumFlags operator~() const
	{
		return EnumFlags(~this->value);
	}

	constexpr bool all(EnumFlags other) const
	{
		return (this->value & other.value) == other.value;
	}

	constexpr bool any(EnumFlags other) const
	{
		return (this->value & other.value) != 0;
	}

	constexpr explicit operator bool() const
	{
		return this->value != 0;
	}
};

template<typename EnumType>
constexpr EnumFlags<EnumType> operator|(EnumType a, EnumType b)
{
	return EnumFlags<EnumType>(a) | EnumFlags<EnumType>(b);
}

template<typename EnumType>
constexpr EnumFlags<EnumType> operator&(EnumType a, EnumType b)
{
	return EnumFlags<EnumType>(a) & EnumFlags<EnumType>(b);
}

template<typename EnumType>
constexpr EnumFlags<EnumType> operator&(EnumType a, EnumFlags<EnumType> b)
{
	return EnumFlags<EnumType>(a) & b;
}

#endif
