PROJECT(components CXX)

FILE(GLOB COMP_ARCHIVES
	archives/*.h
	archives/*.hpp
	archives/*.cpp)

FILE(GLOB COMP_DEBUG
	debug/*.h
	debug/*.hpp
	debug/*.cpp)

FILE(GLOB COMP_DOS
	dos/*.h
	dos/*.hpp
	dos/*.cpp)

FILE(GLOB COMP_UTILITIES
	utilities/*.h
	utilities/*.hpp
	utilities/*.cpp)

FILE(GLOB COMP_VFS
	vfs/*.h
	vfs/*.hpp
	vfs/*.cpp)

SET(COMP_SOURCES
	${COMP_ARCHIVES}
	${COMP_DEBUG}
	${COMP_DOS}
	${COMP_UTILITIES}
	${COMP_VFS})

IF (WIN32)
	FILE(GLOB COMP_WIN32_SOURCES
		misc/*.h
		misc/*.hpp
		misc/*.cpp)

	SET(COMP_SOURCES ${COMP_SOURCES} ${COMP_WIN32_SOURCES})
ENDIF (WIN32)

ADD_LIBRARY (components STATIC ${COMP_SOURCES})

# Visual Studio filters.
SOURCE_GROUP(TREE ${CMAKE_SOURCE_DIR}/components FILES ${COMP_SOURCES})
