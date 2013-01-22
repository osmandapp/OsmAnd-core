project(protobuf_osmand)

set(ROOT "${OSMAND_ROOT}/externals/protobuf")
set(UPSTREAM "${ROOT}/upstream.patched")
include_directories(AFTER SYSTEM
	"${ROOT}"
	"${UPSTREAM}/src"
)
add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
add_library(protobuf_osmand STATIC
    "${UPSTREAM}/src/google/protobuf/extension_set.cc"
	"${UPSTREAM}/src/google/protobuf/generated_message_util.cc"
	"${UPSTREAM}/src/google/protobuf/io/coded_stream.cc"
	"${UPSTREAM}/src/google/protobuf/io/zero_copy_stream.cc"
	"${UPSTREAM}/src/google/protobuf/io/zero_copy_stream_impl.cc"
	"${UPSTREAM}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc"
	"${UPSTREAM}/src/google/protobuf/message_lite.cc"
	"${UPSTREAM}/src/google/protobuf/repeated_field.cc"
	"${UPSTREAM}/src/google/protobuf/stubs/common.cc"
	"${UPSTREAM}/src/google/protobuf/stubs/once.cc"
	"${UPSTREAM}/src/google/protobuf/wire_format_lite.cc"
)
