project(sqlite)

prepare_upstream()

add_library(sqlite_static STATIC
	"upstream.patched/sqlite3.c"
)
target_compile_definitions(sqlite_static
	PRIVATE
		-DSQLITE_ENABLE_RTREE
		-DSQLITE_ENABLE_FTS4
		-DSQLITE_ENABLE_FTS5
		-DSQLITE_ENABLE_JSON1
		-DSQLITE_ENABLE_RBU
		-DSQLITE_ENABLE_STAT4
)
target_include_directories(sqlite_static
	PUBLIC
		"upstream.patched"
)
