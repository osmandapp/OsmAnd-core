project(icu4c)

prepare_upstream()

file(GLOB_RECURSE source_common "upstream.patched/icu4c/source/common/*.[ch]*")
set_source_files_properties(${source_common}
	PROPERTIES
		COMPILE_DEFINITIONS "U_COMMON_IMPLEMENTATION"
)

file(GLOB_RECURSE source_i18n "upstream.patched/icu4c/source/i18n/*.[ch]*")
set_source_files_properties(${source_i18n}
	PROPERTIES
		COMPILE_DEFINITIONS "U_I18N_IMPLEMENTATION"
)

add_library(icu4c_static STATIC
	${source_common}
	${source_i18n}
	"upstream.patched/icu4c/source/stubdata/stubdata.c"
)

set(target_specific_private_definitions "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
			-D_CRT_NONSTDC_NO_WARNINGS
		)
	endif()
elseif (CMAKE_TARGET_OS STREQUAL "android")
	set(target_specific_private_definitions ${target_specific_private_definitions}
		-DU_HAVE_NL_LANGINFO_CODESET=0
	)
endif()

target_compile_definitions(icu4c_static
	PRIVATE
		-D_OSMAND_DUMMY_
		${target_specific_private_definitions}
	PUBLIC
		-DU_STATIC_IMPLEMENTATION
		-DU_LIB_SUFFIX_C_NAME=_osmand
		-DUCONFIG_NO_FILE_IO=1
		-DUCONFIG_NO_SERVICE=1
)

target_include_directories(icu4c_static
	PUBLIC
		"upstream.patched/icu4c/source/common"
		"upstream.patched/icu4c/source/i18n"
)
set_property(TARGET icu4c_static
	PROPERTY
		CXX_STANDARD 11
)
if (CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
	# NOTE: Workaround https://gitlab.kitware.com/cmake/cmake/-/issues/19713
	set_property(TARGET icu4c_static
		PROPERTY
			XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11"
	)
endif()
