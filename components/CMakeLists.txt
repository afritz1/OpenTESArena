PROJECT(components CXX)

SET(COMP_ARCHIVES
	"archives/archive.cpp"
	"archives/archive.hpp"
	"archives/bsaarchive.cpp"
	"archives/bsaarchive.hpp")

SET(COMP_DEBUG
	"debug/Debug.cpp"
	"debug/Debug.h")

SET(COMP_DOS
	"dos/DOSUtils.h")

SET(COMP_UTILITIES
	"utilities/Buffer.h"
	"utilities/Buffer2D.h"
	"utilities/Buffer3D.h"
	"utilities/BumpAllocator.h"
	"utilities/Bytes.cpp"
	"utilities/Bytes.h"
	"utilities/Directory.cpp"
	"utilities/Directory.h"
	"utilities/Enum.h"
	"utilities/File.cpp"
	"utilities/File.h"
	"utilities/FixedPool.h"
	"utilities/FlatMap.h"
	"utilities/FPSCounter.cpp"
	"utilities/FPSCounter.h"
	"utilities/Heap.cpp"
	"utilities/Heap.h"
	"utilities/HexPrinter.cpp"
	"utilities/HexPrinter.h"
	"utilities/KeyValueFile.cpp"
	"utilities/KeyValueFile.h"
	"utilities/KeyValuePool.h"
	"utilities/ObjFile.cpp"
	"utilities/ObjFile.h"
	"utilities/Path.cpp"
	"utilities/Path.h"
	"utilities/Profiler.cpp"
	"utilities/Profiler.h"
	"utilities/Singleton.h"
	"utilities/Span.h"
	"utilities/Span2D.h"
	"utilities/Span3D.h"
	"utilities/StaticVector.h"
	"utilities/String.cpp"
	"utilities/String.h"
	"utilities/StringView.cpp"
	"utilities/StringView.h"
	"utilities/TextLinesFile.cpp"
	"utilities/TextLinesFile.h"
	"utilities/VirtualHeap.cpp"
	"utilities/VirtualHeap.h")

SET(COMP_VFS
	"vfs/dirent.h"
	"vfs/manager.cpp"
	"vfs/manager.hpp")

SET(COMP_SOURCES
	${COMP_ARCHIVES}
	${COMP_DEBUG}
	${COMP_DOS}
	${COMP_UTILITIES}
	${COMP_VFS})

IF (WIN32)
	SET(COMP_WIN32_SOURCES
		"misc/fnmatch.cpp"
		"misc/fnmatch.h")

	SET(COMP_SOURCES ${COMP_SOURCES} ${COMP_WIN32_SOURCES})
ENDIF (WIN32)

ADD_LIBRARY(components STATIC ${COMP_SOURCES})

# Visual Studio filters.
SOURCE_GROUP(TREE ${CMAKE_SOURCE_DIR}/components FILES ${COMP_SOURCES})
