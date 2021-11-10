project(protobuf)

prepare_upstream()

if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
	configure_file("config.h.ios" "${CMAKE_CURRENT_BINARY_DIR}/config.h" COPYONLY)
else()
	save_and_set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${CMAKE_C_FLAGS}")
	include(CheckIncludeFile)
	include(CheckIncludeFiles)

	check_include_file("hash_map" _HAVE_HASH_MAP)
	if (_HAVE_HASH_MAP)
		set(HAVE_HASH_MAP ON)
		set(HASH_MAP_CLASS "hash_map")
		set(HASH_MAP_H "<hash_map>")
		set(HASH_NAMESPACE "std")
	endif ()

	check_include_file("unordered_map" _HAVE_UNORDERED_MAP)
	if (_HAVE_UNORDERED_MAP)
		set(HAVE_HASH_MAP ON)
		set(HASH_MAP_CLASS "unordered_map")
		set(HASH_MAP_H "<unordered_map>")
		set(HASH_NAMESPACE "std")
	endif ()

	check_include_file("hash_set" _HAVE_HASH_SET)
	if (_HAVE_HASH_SET)
		set(HAVE_HASH_SET ON)
		set(HASH_SET_CLASS "hash_set")
		set(HASH_SET_H "<hash_set>")
		set(HASH_NAMESPACE "std")
	endif ()

	check_include_file("unordered_set" _HAVE_UNORDERED_SET)
	if (_HAVE_UNORDERED_SET)
		set(HAVE_HASH_SET ON)
		set(HASH_SET_CLASS "unordered_set")
		set(HASH_SET_H "<unordered_set>")
		set(HASH_NAMESPACE "std")
	endif ()

	check_include_file("dlfcn.h" HAVE_DLFCN_H)
	check_include_file("fcntl.h" HAVE_FCNTL_H)
	check_function_exists("ftruncate" HAVE_FTRUNCATE)
	check_include_file("inttypes.h" HAVE_INTTYPES_H)
	check_include_file("limits.h" HAVE_LIMITS_H)
	check_include_file("memory.h" HAVE_MEMORY_H)
	check_function_exists("memset" HAVE_MEMSET)
	check_function_exists("mkdir" HAVE_MKDIR)
	check_include_file("pthread.h" HAVE_PTHREAD)
	check_include_file("stdint.h" HAVE_STDINT_H)
	check_include_file("stdlib.h" HAVE_STDLIB_H)
	check_function_exists("strchr" HAVE_STRCHR)
	check_function_exists("strerror" HAVE_STRERROR)
	check_include_file("strings.h" HAVE_STRINGS_H)
	check_include_file("string.h" HAVE_STRING_H)
	check_function_exists("strtol" HAVE_STRTOL)
	check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
	check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
	check_include_file("unistd.h" HAVE_UNISTD_H)
	set(HAVE_ZLIB ON)
	unset(PTHREAD_CREATE_JOINABLE)
	check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)

	restore(CMAKE_REQUIRED_FLAGS)

	configure_file("config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/config.h")
endif()

add_library(protobuf_static STATIC
	"upstream.patched/src/google/protobuf/descriptor.cc"
	"upstream.patched/src/google/protobuf/descriptor_database.cc"
	"upstream.patched/src/google/protobuf/descriptor.pb.cc"
	"upstream.patched/src/google/protobuf/dynamic_message.cc"
	"upstream.patched/src/google/protobuf/extension_set.cc"
	"upstream.patched/src/google/protobuf/extension_set_heavy.cc"
	"upstream.patched/src/google/protobuf/generated_message_reflection.cc"
	"upstream.patched/src/google/protobuf/generated_message_util.cc"
	"upstream.patched/src/google/protobuf/message.cc"
	"upstream.patched/src/google/protobuf/message_lite.cc"
	"upstream.patched/src/google/protobuf/reflection_ops.cc"
	"upstream.patched/src/google/protobuf/repeated_field.cc"
	"upstream.patched/src/google/protobuf/service.cc"
	"upstream.patched/src/google/protobuf/text_format.cc"
	"upstream.patched/src/google/protobuf/unknown_field_set.cc"
	"upstream.patched/src/google/protobuf/wire_format.cc"
	"upstream.patched/src/google/protobuf/wire_format_lite.cc"
	"upstream.patched/src/google/protobuf/io/coded_stream.cc"
	"upstream.patched/src/google/protobuf/stubs/common.cc"
	"upstream.patched/src/google/protobuf/stubs/once.cc"
	"upstream.patched/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc"
	"upstream.patched/src/google/protobuf/stubs/stringprintf.cc"
	"upstream.patched/src/google/protobuf/stubs/structurally_valid.cc"
	"upstream.patched/src/google/protobuf/stubs/strutil.cc"
	"upstream.patched/src/google/protobuf/stubs/substitute.cc"
	"upstream.patched/src/google/protobuf/io/zero_copy_stream_impl.cc"
	"upstream.patched/src/google/protobuf/io/printer.cc"
	"upstream.patched/src/google/protobuf/io/tokenizer.cc"
#	"upstream.patched/src/google/protobuf/io/gzip_stream.cc"
	"upstream.patched/src/google/protobuf/io/coded_stream.cc"
	"upstream.patched/src/google/protobuf/io/zero_copy_stream_impl_lite.cc"
	"upstream.patched/src/google/protobuf/io/zero_copy_stream.cc"
)

set(target_specific_private_definitions "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
			-D_CRT_NONSTDC_NO_WARNINGS
		)
	endif()
endif()

target_compile_definitions(protobuf_static
	PRIVATE
		-D__OSMAND_DUMMY__
		${target_specific_private_definitions}
)
target_include_directories(protobuf_static
	PUBLIC
		"upstream.patched/src"
		"${CMAKE_CURRENT_BINARY_DIR}"
)
