project(zlib)

add_library(z_static STATIC
	"upstream.patched/adler32.c"
	"upstream.patched/compress.c"
	"upstream.patched/crc32.c"
	"upstream.patched/deflate.c"
	"upstream.patched/gzclose.c"
	"upstream.patched/gzlib.c"
	"upstream.patched/gzread.c"
	"upstream.patched/gzwrite.c"
	"upstream.patched/infback.c"
	"upstream.patched/inffast.c"
	"upstream.patched/inflate.c"
	"upstream.patched/inftrees.c"
	"upstream.patched/trees.c"
	"upstream.patched/uncompr.c"
	"upstream.patched/zutil.c"
)

target_compile_definitions(z_static
	PRIVATE
		-DHAVE_SYS_TYPES_H
		-DHAVE_STDINT_H
		-DHAVE_STDDEF_H
	PUBLIC
		-DZ_PREFIX
)
target_include_directories(z_static
	PUBLIC
		"upstream.patched"
)
