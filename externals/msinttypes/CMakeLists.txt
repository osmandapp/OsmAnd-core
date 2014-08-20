project(msinttypes)

prepare_upstream()

add_library(msinttypes_headers_only INTERFACE)
target_include_directories(msinttypes_headers_only
	INTERFACE
		"."
)
