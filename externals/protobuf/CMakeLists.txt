project(protobuf_osmand)

include_directories(AFTER SYSTEM
	"."
	"upstream.patched/src"
)
add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()
add_library(protobuf_osmand STATIC
    "upstream.patched/src/google/protobuf/extension_set.cc"
	"upstream.patched/src/google/protobuf/generated_message_util.cc"
	"upstream.patched/src/google/protobuf/io/coded_stream.cc"
	"upstream.patched/src/google/protobuf/io/zero_copy_stream.cc"
	"upstream.patched/src/google/protobuf/io/zero_copy_stream_impl.cc"
	"upstream.patched/src/google/protobuf/io/zero_copy_stream_impl_lite.cc"
	"upstream.patched/src/google/protobuf/message_lite.cc"
	"upstream.patched/src/google/protobuf/repeated_field.cc"
	"upstream.patched/src/google/protobuf/stubs/common.cc"
	"upstream.patched/src/google/protobuf/stubs/once.cc"
	"upstream.patched/src/google/protobuf/wire_format_lite.cc"
)
