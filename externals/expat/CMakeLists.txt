project(expat)

add_library(expat_static STATIC
	"upstream.patched/lib/xmlparse.c"
	"upstream.patched/lib/xmlrole.c"
	"upstream.patched/lib/xmltok.c"
	"upstream.patched/lib/xmltok_impl.c"
	"upstream.patched/lib/xmltok_ns.c"
)

target_compile_definitions(expat_static
	PRIVATE
		-DHAVE_EXPAT_CONFIG_H
	PUBLIC
		-DXML_STATIC
)
target_include_directories(expat_static
	PRIVATE
		"."
	PUBLIC
		"upstream.patched/lib"
)
