# External : z
add_subdirectory("${OSMAND_ROOT}/core/externals/zlib" "core/externals/zlib")

# External : png
add_subdirectory("${OSMAND_ROOT}/core/externals/libpng" "core/externals/libpng")
add_dependencies(png_osmand z_osmand)

# External : expat
add_subdirectory("${OSMAND_ROOT}/core/externals/expat" "core/externals/expat")

# External : freetype2
add_subdirectory("${OSMAND_ROOT}/core/externals/freetype" "core/externals/freetype")

# External : gif
add_subdirectory("${OSMAND_ROOT}/core/externals/giflib" "core/externals/giflib")

# External : harfbuzz
add_subdirectory("${OSMAND_ROOT}/core/externals/harfbuzz" "core/externals/harfbuzz")

# External : jpeg
add_subdirectory("${OSMAND_ROOT}/core/externals/jpeg" "core/externals/jpeg")

# External : protobuf
add_subdirectory("${OSMAND_ROOT}/core/externals/protobuf" "core/externals/protobuf")

# External : skia
add_subdirectory("${OSMAND_ROOT}/core/externals/skia" "core/externals/skia")
add_dependencies(skia_osmand png_osmand gif_osmand jpeg_osmand harfbuzz_osmand expat_osmand freetype2_osmand)

# OsmAnd core
add_subdirectory("${OSMAND_ROOT}/core" "core/OsmAndCore")
add_dependencies(OsmAndCore skia_osmand protobuf_osmand)

# OsmAnd core utils
add_subdirectory("${OSMAND_ROOT}/core/utils" "core/OsmAndCoreUtils")
add_dependencies(OsmAndCoreUtils OsmAndCore)