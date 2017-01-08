PROJECT(components CXX)

FILE(GLOB COMP_SOURCES vfs/*.h vfs/*.hpp vfs/*.cpp
	archives/*.h archives/*.hpp archives/*.cpp
)

IF (WIN32)
	FILE(GLOB COMP_WIN32_SOURCES misc/*.h misc/*.c misc/*.hpp misc/*.cpp)
ENDIF (WIN32)

IF (MINGW)
	FILE(GLOB COMP_CL_SOURCES CL/*.h CL/*.hpp CL/*.cpp)
ENDIF (MINGW)

ADD_LIBRARY (components STATIC ${COMP_SOURCES} ${COMP_WIN32_SOURCES} ${COMP_CL_SOURCES})

SET_TARGET_PROPERTIES(components PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS ON
)
