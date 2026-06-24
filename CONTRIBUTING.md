## Contributing

To report bugs, you can use [issues](https://github.com/afritz1/OpenTESArena/issues) or leave a comment on any commit.

If you'd like to contribute code, open a [PR](https://github.com/afritz1/OpenTESArena/pulls). It's better to discuss first in an issue or on Discord. Your contribution should be authored significantly enough by you the human in order to be added to AUTHORS.md.

[Milestones](https://github.com/afritz1/OpenTESArena/milestones) are where the remaining issues for a release are tracked. The [wiki](https://github.com/afritz1/OpenTESArena/wiki) contains the project roadmap and is full of information about how Arena works.

### Coding Style
- C++17 and above
- Containers like `std::string`, `std::vector`, and `std::unordered_map` are allowed.
- Use `const` with everything except for tiny function parameters like `int` and `double` that are passed by value.
- Use `Span<const T>` when passing an array or vector to a function.
- C arrays are okay if used with `DebugAssertIndex()`, `std::size()`, or `Span<T>`.
- Always use the full typename except for long or weird types which should be `auto`.
- Lambdas should capture only what's needed.
- Globals are okay as long as they're usually `const`.
- `-1` is frequently used to mean invalid array index or ID. Checking if the value is `>= 0` is considered correct usage.
- When calling a function with a name like `alloc()`, always release the created ID when finished.
- Always use `i++` instead of `++i`. `std` iterators are a little faster with `++iter` but it's a micro-optimization.
- Don't use curly braces to construct `int`/`double`. Use regular assignment `=`.

### File Structure

Source code filenames are UpperCamelCase and use .h or .cpp extensions.

Typical .h file:
```C++
#pragma once

// Header includes alphabetized within each group.
#include <standard lib 1.h>
#include <standard lib 2.h>

#include "project includes like SDL.h"

#include "LocalSourceFile1.h"
#include "LocalSourceFile2.h"
#include "../SomeCodeFolder/LocalSourceFile3.h"

#include "components/utilities/Span.h"

// Alphabetized forward declarations.
class SomeClass;

enum class SomeEnumClass;

struct SomeStruct;

struct ExampleStruct
{
	int value;
	int index;

	// Default constructor to prevent garbage values.
	ExampleStruct();

	void init(int value, int index);
};

class ExampleClass
{
private:
	int value;
	ExampleStruct exampleStruct;
public:
	ExampleClass(int value, const ExampleStruct &exampleStruct);
	
	static const Vector2 ExampleVec2;
	
	int getCalculatedValue() const;
};
```

Its .cpp file:
```C++
#include <standard lib 3.h>
#include <standard lib 4.h>

#include "ExampleSourceFile.h"
#include "LocalSourceFile4.h"
#include "../SomeFolder/LocalSourceFile5.h"
#include "../SomeFolder/LocalSourceFile6.h"

#include "components/debug/Debug.h"

namespace
{
	// Constants and helper functions only relevant to this file.
}

const Vector2 ExampleClass::ExampleVec2 = Vector2::Zero;

ExampleStruct::ExampleStruct()
{
	this->value = 0;
	this->index = -1; // Default to -1 for array indices and IDs.
}

void ExampleStruct::init(int value, int index)
{
	DebugAssert(index >= 0);
	this->value = value;
	this->index = index;
}

ExampleClass::ExampleClass(int value, const ExampleStruct &exampleStruct)
	: exampleStruct(exampleStruct) // Initializer list for non-trivial types.
{
	this->value = value;
}

int ExampleClass::getCalculatedValue() const
{
	return this->value + this->exampleStruct.value;
}
```

### My Development Environment
- Visual Studio
- CMake GUI
- GitHub Desktop
- DOSBox
- Hex dumps of A.EXE and ACD.EXE
