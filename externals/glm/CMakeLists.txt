project(glm)

prepare_upstream()

add_library(glm_headers_only INTERFACE)
target_include_directories(glm_headers_only
	INTERFACE
		"upstream.patched"
)
