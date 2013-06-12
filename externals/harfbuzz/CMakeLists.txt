project(harfbuzz)

if(MSVC)
	set_source_files_properties("upstream.patched/contrib/harfbuzz-unicode.c" PROPERTIES LANGUAGE CXX)
endif()
add_library(harfbuzz_static STATIC
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

target_include_directories(harfbuzz_static
	PUBLIC
		"upstream.patched/src"
)
