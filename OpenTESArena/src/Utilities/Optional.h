#ifndef OPTIONAL_H
#define OPTIONAL_H

// Required for some compilers that are still behind the times on C++17, like Clang.

#if __has_include(<optional>)
#include <optional>
namespace stdx
{
	using std::optional;
	using std::nullopt_t;
	using std::in_place_t;
	using std::bad_optional_access;
	using std::nullopt;
	using std::in_place;
	using std::make_optional;
}
#elif __has_include(<experimental/optional>)
#include <experimental/optional>
namespace stdx
{
	using std::experimental::optional;
	using std::experimental::nullopt_t;
	using std::experimental::in_place_t;
	using std::experimental::bad_optional_access;
	using std::experimental::nullopt;
	using std::experimental::in_place;
	using std::experimental::make_optional;
}
#endif
#endif
