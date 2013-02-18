# External : z
add_subdirectory("${OSMAND_ROOT}/core/externals/zlib" "zlib")

# External : png
add_subdirectory("${OSMAND_ROOT}/core/externals/libpng" "libpng")
add_dependencies(png_osmand z_osmand)

# External : expat
add_subdirectory("${OSMAND_ROOT}/core/externals/expat" "expat")

# External : freetype2
add_subdirectory("${OSMAND_ROOT}/core/externals/freetype" "freetype")

# External : gif
add_subdirectory("${OSMAND_ROOT}/core/externals/giflib" "giflib")

# External : harfbuzz
add_subdirectory("${OSMAND_ROOT}/core/externals/harfbuzz" "harfbuzz")

# External : jpeg
add_subdirectory("${OSMAND_ROOT}/core/externals/jpeg" "jpeg")

# External : protobuf
add_subdirectory("${OSMAND_ROOT}/core/externals/protobuf" "protobuf")

# External : skia
add_subdirectory("${OSMAND_ROOT}/core/externals/skia" "skia")
add_dependencies(skia_osmand png_osmand gif_osmand jpeg_osmand harfbuzz_osmand expat_osmand freetype2_osmand)

# OsmAnd core
add_subdirectory("${OSMAND_ROOT}/core" "OsmAndCore")
add_dependencies(OsmAndCore skia_osmand protobuf_osmand)
