project(geographiclib)

prepare_upstream()

add_library(geographiclib_headers_only INTERFACE)
target_include_directories(geographiclib_headers_only
	INTERFACE
                "upstream.patched/include"
)
