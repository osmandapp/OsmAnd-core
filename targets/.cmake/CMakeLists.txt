cmake_minimum_required(VERSION 2.8.10 FATAL_ERROR)

set(OSMAND_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
set(OSMAND_PROJECTS_ROOT "${CMAKE_CURRENT_LIST_DIR}/projects")

# Output paths
set(OSMAND_OUTPUT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../binaries/${CMAKE_TARGET_OS}/${CMAKE_TARGET_CPU_ARCH}/${CMAKE_TARGET_CPU_NAME}/${CMAKE_BUILD_TYPE}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OSMAND_OUTPUT_ROOT}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OSMAND_OUTPUT_ROOT}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OSMAND_OUTPUT_ROOT}")

# Include CPU specific settings
if(DEFINED CMAKE_TARGET_CPU_NAME)
	include("cpu-specific/${CMAKE_TARGET_CPU_NAME}.cmake")
endif()

# Take CMAKE_*_COMPILER_FLAGS if needed
if(CMAKE_C_COMPILER_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_COMPILER_FLAGS}")
endif()
if(CMAKE_CXX_COMPILER_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_COMPILER_FLAGS}")
endif()

# External : z
add_subdirectory("${OSMAND_PROJECTS_ROOT}/zlib" "zlib")

# External : png
add_subdirectory("${OSMAND_PROJECTS_ROOT}/libpng" "libpng")
add_dependencies(png z)

# External : expat
add_subdirectory("${OSMAND_PROJECTS_ROOT}/expat" "expat")

# External : freetype2
add_subdirectory("${OSMAND_PROJECTS_ROOT}/freetype" "freetype")

# External : gif
add_subdirectory("${OSMAND_PROJECTS_ROOT}/giflib" "giflib")

# External : harfbuzz
add_subdirectory("${OSMAND_PROJECTS_ROOT}/harfbuzz" "harfbuzz")

# External : jpeg
add_subdirectory("${OSMAND_PROJECTS_ROOT}/jpeg" "jpeg")

# External : protobuf
add_subdirectory("${OSMAND_PROJECTS_ROOT}/protobuf" "protobuf")

# External : skia
add_subdirectory("${OSMAND_PROJECTS_ROOT}/skia" "skia")
add_dependencies(skia z png gif jpeg harfbuzza expat freetype2)

# OsmAnd core
add_subdirectory("${OSMAND_PROJECTS_ROOT}/OsmAndCore" "OsmAndCore")
add_dependencies(osmand	harfbuzz skia protobuf)
