# External : z
add_subdirectory("${OSMAND_ROOT}/core/externals/zlib" "core/externals/zlib")

# External : png
add_subdirectory("${OSMAND_ROOT}/core/externals/libpng" "core/externals/libpng")

# External : expat
add_subdirectory("${OSMAND_ROOT}/core/externals/expat" "core/externals/expat")

if(CMAKE_TARGET_OS STREQUAL "linux")
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

if(CMAKE_TARGET_OS STREQUAL "linux" OR CMAKE_TARGET_OS STREQUAL "darwin" OR CMAKE_TARGET_OS STREQUAL "windows")
	# External : OpenGL GLEW
	add_subdirectory("${OSMAND_ROOT}/core/externals/glew" "core/externals/glew")
endif()

# External : GLSL optimizer
add_subdirectory("${OSMAND_ROOT}/core/externals/glsl-optimizer" "core/externals/glsl-optimizer")

# OsmAnd Core
add_subdirectory("${OSMAND_ROOT}/core" "core/OsmAndCore")

# OsmAnd Core Utils
add_subdirectory("${OSMAND_ROOT}/core/utils" "core/OsmAndCoreUtils")
