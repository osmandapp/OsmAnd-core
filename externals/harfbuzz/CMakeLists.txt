project(harfbuzz)

include_directories(AFTER SYSTEM
	"upstream.patched/src"
)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
if(MSVC)
	set_source_files_properties("upstream.patched/contrib/harfbuzz-unicode.c" PROPERTIES LANGUAGE CXX)
endif()
add_library(harfbuzz_osmand STATIC
	"upstream.patched/src/harfbuzz-arabic.c"
	"upstream.patched/src/harfbuzz-buffer.c"
	"upstream.patched/src/harfbuzz-dump.c"
	"upstream.patched/src/harfbuzz-gdef.c"
	"upstream.patched/src/harfbuzz-gpos.c"
	"upstream.patched/src/harfbuzz-greek.c"
	"upstream.patched/src/harfbuzz-gsub.c"
	"upstream.patched/src/harfbuzz-hangul.c"
	"upstream.patched/src/harfbuzz-hebrew.c"
	"upstream.patched/src/harfbuzz-impl.c"
	"upstream.patched/src/harfbuzz-indic.cpp"
	"upstream.patched/src/harfbuzz-khmer.c"
	"upstream.patched/src/harfbuzz-myanmar.c"
	"upstream.patched/src/harfbuzz-open.c"
	"upstream.patched/src/harfbuzz-shaper.cpp"
	"upstream.patched/src/harfbuzz-stream.c"
	"upstream.patched/src/harfbuzz-thai.c"
	"upstream.patched/src/harfbuzz-tibetan.c"
	"upstream.patched/contrib/harfbuzz-unicode.c"
	"upstream.patched/contrib/harfbuzz-unicode-tables.c"
)
