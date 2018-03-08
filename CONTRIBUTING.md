## Contributing

If you'd like to report a problem or a bug you've found, feel free to use the [Issues](https://github.com/afritz1/OpenTESArena/issues) tab or leave a comment on a commit. If you'd like to contribute something, open a [pull request](https://github.com/afritz1/OpenTESArena/pulls) and I'll take a look at it. You can also take a look at the [Projects](https://github.com/afritz1/OpenTESArena/projects) and the [Wiki](https://github.com/afritz1/OpenTESArena/wiki) for the current to-do's and the project roadmap. I'm not really accepting feature requests yet because there are so many low-level components left to do first!

### File Structure
All source filenames are UpperCamelCase and use .h or .cpp extensions. 

This is the typical layout of the C++ header files:
```C++
#include <standard lib 1.h>
#include <standard lib 2.h>

#include "dependencies like SDL.h"

#include "LocalSourceFile1.h"
#include "LocalSourceFile2.h"
#include "../SomeFolder/LocalSourceFile3.h"

// Forward declarations.
class SomeClass;

enum class SomeEnumClass;

class ExampleClass
{
private:
	// Private members.
	
	// Private methods.
public:
	// Constructors.
	// Destructor.
	
	// Static const class declarations.
	
	// Public methods (getters first, setters and voids second).
};
```

And the .cpp files:
```C++
#include <standard lib 3.h>
#include <standard lib 4.h>

#include "ExampleClass.h"
#include "LocalSourceFile4.h"
#include "../SomeFolder/LocalSourceFile5.h"
#include "../SomeFolder/LocalSourceFile6.h"

namespace
{
	// Constant global data relevant to this file.
}

// Static const class definitions.

ExampleClass::ExampleClass(/* Args. */)
	// Initializer list for non-trivial classes and structs.
{
	// Primitive initializations.
}

ExampleClass::~ExampleClass()
{
	// Usually empty, except for SDL types.
}

// Getters.

// Setters and voids.
```

### Coding Style
We're using C++14, so we regularly use all of the nice features like:
- `std::array`, `std::string`, `std::unique_ptr`, `std::unordered_map`, `std::vector`
- `nullptr`
- Enum classes
- Lambdas
- Bounds-checking (i.e., `std::vector::at()`)
- `auto` (only for really long names. It's usually better to just write the type anyway)
- Range-based for loops (i.e., `for (const auto &obj : someVector)`)
- Move semantics via `std::move()`

#### Other Notes
- Prefer pure functions and immutable data.
- Avoid mutable globals and static variables.
- `new` and `delete` should never be necessary. `std::make_unique<T>()` takes care of that for us.
- Use post-increment (`i++`) everywhere except with iterators, in which case use pre-increment (`++iter`) to avoid unnecessary copies.
- Code should work in both 32-bit and 64-bit, and on Windows and Linux (fortunately, SDL2 and the C++14 standard library take care of a lot of this for us).
- Alphabetize each group of header includes. This makes it much easier to find the one you're looking for (via binary search!).
- Personally, I use `this` for accessing class members. It is very convenient to use with Visual Studio's auto-complete, it keeps local variables unambiguous, and I've never needed to mangle the names of class members with sigils.
- Follow the formatting of nearby code (i.e., if it uses tabs instead of spaces, then use tabs).

### My Development Environment
- Windows 7 x64
- Visual Studio 2017
- CMake GUI
- GitHub Desktop for Windows
- DOSBox
