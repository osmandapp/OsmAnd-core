project(icu4c)

file(GLOB_RECURSE source_common "upstream.patched/source/common/*.[ch]*")
set_source_files_properties(${source_common}
	PROPERTIES
		COMPILE_DEFINITIONS "U_COMMON_IMPLEMENTATION"
)

file(GLOB_RECURSE source_io "upstream.patched/source/io/*.[ch]*")
set_source_files_properties(${source_io}
	PROPERTIES
		COMPILE_DEFINITIONS "U_IO_IMPLEMENTATION"
)

file(GLOB_RECURSE source_i18n "upstream.patched/source/i18n/*.[ch]*")
set_source_files_properties(${source_i18n}
	PROPERTIES
		COMPILE_DEFINITIONS "U_I18N_IMPLEMENTATION"
)

file(GLOB_RECURSE source_layout "upstream.patched/source/layout/*.[ch]*")
set_source_files_properties(${source_layout}
	PROPERTIES
		COMPILE_DEFINITIONS "U_LAYOUT_IMPLEMENTATION"
)

add_library(icu4c_static STATIC
	${source_common}
	${source_io}
	${source_i18n}
	${source_layout}
)

set(target_specific_private_definitions "")
if(CMAKE_TARGET_OS STREQUAL "windows")
	if(CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
			-D_CRT_NONSTDC_NO_WARNINGS
		)
	endif()
endif()

target_compile_definitions(icu4c_static
	PRIVATE
		${target_specific_private_definitions}
	PUBLIC
		-DU_STATIC_IMPLEMENTATION
)

target_include_directories(icu4c_static
	PUBLIC
		"upstream.patched/source/common/unicode"
		"upstream.patched/source/io/unicode"
		"upstream.patched/source/i18n/unicode"
	PRIVATE
		"upstream.patched/source/common"
		"upstream.patched/source/io"
		"upstream.patched/source/i18n"
		"upstream.patched/source/layout"
)
