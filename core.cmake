# External : z
add_subdirectory("${OSMAND_ROOT}/core/externals/zlib" "core/externals/zlib")

# External : png
add_subdirectory("${OSMAND_ROOT}/core/externals/libpng" "core/externals/libpng")

# External : expat
add_subdirectory("${OSMAND_ROOT}/core/externals/expat" "core/externals/expat")

if (CMAKE_TARGET_OS STREQUAL "linux" OR
	CMAKE_TARGET_OS STREQUAL "android")
	# External : freetype2
	add_subdirectory("${OSMAND_ROOT}/core/externals/freetype" "core/externals/freetype")
endif()

# External : gif
add_subdirectory("${OSMAND_ROOT}/core/externals/giflib" "core/externals/giflib")

# External : jpeg
add_subdirectory("${OSMAND_ROOT}/core/externals/jpeg" "core/externals/jpeg")

# External : protobuf
add_subdirectory("${OSMAND_ROOT}/core/externals/protobuf" "core/externals/protobuf")

# External : skia
add_subdirectory("${OSMAND_ROOT}/core/externals/skia" "core/externals/skia")

# External : GDAL
add_subdirectory("${OSMAND_ROOT}/core/externals/gdal" "core/externals/gdal")

if (CMAKE_TARGET_OS STREQUAL "linux" OR
	CMAKE_TARGET_OS STREQUAL "macosx" OR
	CMAKE_TARGET_OS STREQUAL "windows")
	# External : OpenGL GLEW
	add_subdirectory("${OSMAND_ROOT}/core/externals/glew" "core/externals/glew")
endif()


# External: ICU4C
add_subdirectory("${OSMAND_ROOT}/core/externals/icu4c" "core/externals/icu4c")

# External: libarchive
add_subdirectory("${OSMAND_ROOT}/core/externals/libarchive" "core/externals/libarchive")

# External: glm
add_subdirectory("${OSMAND_ROOT}/core/externals/glm" "core/externals/glm")

# External: boost
add_subdirectory("${OSMAND_ROOT}/core/externals/boost" "core/externals/boost")

# OsmAnd Core
add_subdirectory("${OSMAND_ROOT}/core" "core")

# OsmAnd Core wrappers
include("${OSMAND_ROOT}/core/wrappers/wrappers.cmake")

# OsmAnd Core Tools
include("${OSMAND_ROOT}/core/tools/tools.cmake")
