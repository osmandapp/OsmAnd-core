project(gdal)

set(target_specific_sources "")
set(target_specific_public_libraries "")
set(target_specific_private_definitions "")

if (CMAKE_TARGET_OS STREQUAL "ios" AND CMAKE_TARGET_BUILD_TOOL STREQUAL "xcode")
	configure_file("cpl_config.h.ios" "${CMAKE_CURRENT_BINARY_DIR}/cpl_config.h" COPYONLY)
elseif (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
	configure_file("upstream.patched/port/cpl_config.h.vc" "${CMAKE_CURRENT_BINARY_DIR}/cpl_config.h" COPYONLY)
else()
	include(CheckIncludeFile)
	include(CheckIncludeFiles)
	include(CheckFunctionExists)
	include(CheckSymbolExists)
	include(CheckLibraryExists)
	include(CheckTypeSize)
	include(TestBigEndian)
	include(CheckCSourceCompiles) 
	include(CheckCXXSourceCompiles) 
	
	check_include_file("pthread.h" CPL_MULTIPROC_PTHREAD)
	check_symbol_exists("PTHREAD_MUTEX_RECURSIVE" "pthread.h" PTHREAD_MUTEX_RECURSIVE)
	unset(GDAL_PREFIX)
	check_include_file("assert.h" HAVE_ASSERT_H)
	check_function_exists("atoll" HAVE_ATOLL)
	check_include_file("csf.h" HAVE_CSF_H)
	check_include_file("dbmalloc.h" HAVE_DBMALLOC_H)
	check_symbol_exists("strtof" "stdlib.h" HAVE_DECL_STRTOF)
	check_include_file("direct.h" HAVE_DIRECT_H)
	check_include_file("dlfcn.h" HAVE_DLFCN_H)
	check_function_exists("_doprnt" HAVE_DOPRNT)
	check_include_file("errno.h" HAVE_ERRNO_H)
	check_include_file("fcntl.h" HAVE_FCNTL_H)
	check_include_file("float.h" HAVE_FLOAT_H)
	check_function_exists("getcwd" HAVE_GETCWD)
	check_function_exists("iconv" HAVE_ICONV)
	set(HAVE_IEEEFP ON)
	check_type_size("int8" SIZEOF_INT8)
	check_type_size("int16" SIZEOF_INT16)
	if (SIZEOF_INT16)
		set(HAVE_INT16 ON)
	else()
		set(HAVE_INT16 OFF)
	endif()
	check_type_size("int32" SIZEOF_INT32)
	if (SIZEOF_INT32)
		set(HAVE_INT32 ON)
	else()
		set(HAVE_INT32 OFF)
	endif()
	if (SIZEOF_INT8)
		set(HAVE_INT8 ON)
	else()
		set(HAVE_INT8 OFF)
	endif()
	check_include_file("inttypes.h" HAVE_INTTYPES_H)
	check_include_file("jpeglib.h" HAVE_JPEGLIB_H) #NOTE: But we have libjpeg, no extra check is needed
	check_library_exists("dl" "dlopen" "" HAVE_LIBDL)
	check_library_exists("m" "sin" "" HAVE_LIBM)
	set(HAVE_LIBPQ NO)
	check_library_exists("rt" "fork" "" HAVE_LIBRT)
	check_include_file("limits.h" HAVE_LIMITS_H)
	check_include_file("locale.h" HAVE_LOCALE_H)
	check_type_size("long long" SIZEOF_LONG_LONG)
	if (SIZEOF_LONG_LONG)
		set(HAVE_LONG_LONG ON)
	else()
		set(HAVE_LONG_LONG OFF)
	endif()
	check_include_file("memory.h" HAVE_MEMORY_H)
	check_include_file("png.h" HAVE_PNG_H) #NOTE: But we have libpng, no extra check is needed
	check_function_exists("snprintf" HAVE_SNPRINTF)
	check_include_file("stdint.h" HAVE_STDINT_H)
	check_include_file("stdlib.h" HAVE_STDLIB_H)
	check_include_file("strings.h" HAVE_STRINGS_H)
	check_include_file("string.h" HAVE_STRING_H)
	check_function_exists("strtof" HAVE_STRTOF)
	check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
	check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
	check_include_file("unistd.h" HAVE_UNISTD_H)
	check_include_file("values.h" HAVE_VALUES_H)
	check_function_exists("vprintf" HAVE_VPRINTF)
	check_function_exists("vsnprintf" HAVE_VSNPRINTF)
	check_function_exists("readlink" HAVE_READLINK)
	check_function_exists("posix_spawnp" HAVE_POSIX_SPAWNP)
	check_function_exists("vfork" HAVE_VFORK)
	check_function_exists("lstat" HAVE_LSTAT)
	if (HAVE_ICONV)
		# Check if iconv() takes const char ** (C)
		check_c_source_compiles("
			#include <iconv.h>
			int main(int argc, char ** argv)
			{ return iconv(0, (const char **)0, 0, (char**)0, 0); }
			" ICONV_NEEDS_C_CONST)
		# Check if iconv() takes char ** (C)
		check_c_source_compiles("
			#include <iconv.h>
			int main(int argc, char ** argv)
			{ return iconv(0, (char **)0, 0, (char**)0, 0); }
			" ICONV_NEEDS_C_NON_CONST)
		if (ICONV_NEEDS_C_CONST AND NOT ICONV_NEEDS_C_NON_CONST)
			set(ICONV_CONST "const")
		elseif (NOT ICONV_NEEDS_C_CONST AND ICONV_NEEDS_C_NON_CONST)
			set(ICONV_CONST " ")
		elseif (ICONV_NEEDS_C_CONST AND ICONV_NEEDS_C_NON_CONST)
			# Doesn't matter
			set(ICONV_CONST " ")
		else()
			message(FATAL_ERROR "iconv.h: Take a look on iconv() declaration")
		endif()
		message("ICONV_CONST set to '${ICONV_CONST}'")

		# Check if iconv() takes const char ** (C++)
		check_cxx_source_compiles("
			#include <iconv.h>
			int main(int argc, char ** argv)
			{ return iconv(0, (const char **)0, 0, (char**)0, 0); }
			" ICONV_NEEDS_CPP_CONST)
		# Check if iconv() takes char ** (C++)
		check_cxx_source_compiles("
			#include <iconv.h>
			int main(int argc, char ** argv)
			{ return iconv(0, (char **)0, 0, (char**)0, 0); }
			" ICONV_NEEDS_CPP_NON_CONST)
		if (ICONV_NEEDS_CPP_CONST AND NOT ICONV_NEEDS_CPP_NON_CONST)
			set(ICONV_CPP_CONST "const")
		elseif (NOT ICONV_NEEDS_CPP_CONST AND ICONV_NEEDS_CPP_NON_CONST)
			set(ICONV_CPP_CONST " ")
		elseif (ICONV_NEEDS_CPP_CONST AND ICONV_NEEDS_CPP_NON_CONST)
			# Doesn't matter
			set(ICONV_CPP_CONST " ")
		else()
			message(FATAL_ERROR "iconv.h: Take a look on iconv() declaration")
		endif()
		message("ICONV_CPP_CONST set to '${ICONV_CPP_CONST}'")
	else()
		unset(ICONV_CONST)
		unset(ICONV_CPP_CONST)
	endif()
	unset(LT_OBJDIR)
	set(MACOSX_FRAMEWORK OFF)
	check_type_size("int" SIZEOF_INT)
	check_type_size("long" SIZEOF_LONG)
	check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
	check_type_size("void*" SIZEOF_VOIDP)
	check_include_files("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
	check_function_exists("fseek64" UNIX_STDIO_64)
	unset(USE_GCC_VISIBILITY_FLAG)
	set(HAVE_GCC_ATOMIC_BUILTINS OFF)
	unset(VSI_FOPEN64)
	unset(VSI_FTRUNCATE64)
	unset(VSI_FSEEK64)
	unset(VSI_FTELL64)
	unset(VSI_LARGE_API_SUPPORTED)
	unset(VSI_NEED_LARGEFILE64_SOURCE)
	unset(VSI_STAT64)
	unset(VSI_STAT64_T)
	test_big_endian(WORDS_BIGENDIAN)
	if (WORDS_BIGENDIAN)
		set(HOST_FILLORDER "FILLORDER_MSB2LSB")
	else()
		set(HOST_FILLORDER "FILLORDER_LSB2MSB")
	endif()
	check_function_exists("getaddrinfo" HAVE_GETADDRINFO)

	configure_file("cpl_config.h.cmake" "${CMAKE_CURRENT_BINARY_DIR}/cpl_config.h")
endif()

# Target specific
if (CMAKE_TARGET_OS STREQUAL "windows")
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/port/cpl_vsil_win32.cpp"
	)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		Ws2_32
	)

	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
			-D_CRT_NONSTDC_NO_WARNINGS
		)
	endif()
else()
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/port/cpl_vsil_unix_stdio_64.cpp"
	)
endif()

file(GLOB_RECURSE gdal_gcore_sources "upstream.patched/gcore/*.cpp")
file(GLOB gdal_gcore_sources__remove
	"upstream.patched/gcore/gdaldllmain.cpp")
list(REMOVE_ITEM gdal_gcore_sources ${gdal_gcore_sources__remove})
file(GLOB_RECURSE gdal_alg_sources "upstream.patched/alg/*.cpp")
file(GLOB gdal_ogr_sources "upstream.patched/ogr/*.cpp")
file(GLOB_RECURSE gdal_ogr_frmts_mem_sources "upstream.patched/ogr/ogrsf_frmts/mem/*.cpp")
file(GLOB gdal_frmts_sources "upstream.patched/frmts/*.c*")
file(GLOB_RECURSE gdal_frmts_gtiff_sources "upstream.patched/frmts/gtiff/*.c*")

add_library(gdal_static STATIC
	"cpl_config.h"
	${gdal_gcore_sources}
	
	${gdal_alg_sources}
	"upstream.patched/alg/gdal_crs.c"
	"upstream.patched/alg/gdalwarpkernel_opencl.c"

	${gdal_ogr_sources}
	${gdal_ogr_frmts_mem_sources}
	${gdal_frmts_sources}
	${gdal_frmts_gtiff_sources}
	
	"upstream.patched/frmts/hfa/hfaband.cpp"
	"upstream.patched/frmts/hfa/hfadataset.cpp"
	"upstream.patched/frmts/hfa/hfadictionary.cpp"
	"upstream.patched/frmts/hfa/hfaentry.cpp"
	"upstream.patched/frmts/hfa/hfafield.cpp"
	"upstream.patched/frmts/hfa/hfaopen.cpp"
	"upstream.patched/frmts/hfa/hfatype.cpp"
	"upstream.patched/frmts/hfa/hfacompress.cpp"
	"upstream.patched/frmts/hfa/hfa_overviews.cpp"

	"upstream.patched/port/cpl_atomic_ops.cpp"
	"upstream.patched/port/cpl_base64.cpp"
	"upstream.patched/port/cpl_conv.cpp"
	"upstream.patched/port/cpl_csv.cpp"
	"upstream.patched/port/cpl_error.cpp"
	"upstream.patched/port/cpl_findfile.cpp"
	"upstream.patched/port/cpl_getexecpath.cpp"
	"upstream.patched/port/cpl_google_oauth2.cpp"
	"upstream.patched/port/cpl_hash_set.cpp"
	"upstream.patched/port/cpl_http.cpp"
	"upstream.patched/port/cpl_list.cpp"
	"upstream.patched/port/cpl_minixml.cpp"
	"upstream.patched/port/cpl_minizip_ioapi.cpp"
	"upstream.patched/port/cpl_minizip_unzip.cpp"
	"upstream.patched/port/cpl_minizip_zip.cpp"
	"upstream.patched/port/cpl_multiproc.cpp"
	"upstream.patched/port/cpl_path.cpp"
	"upstream.patched/port/cpl_progress.cpp"
	"upstream.patched/port/cpl_quad_tree.cpp"
	"upstream.patched/port/cpl_recode.cpp"
	"upstream.patched/port/cpl_recode_iconv.cpp"
	"upstream.patched/port/cpl_recode_stub.cpp"
	"upstream.patched/port/cpl_spawn.cpp"
	"upstream.patched/port/cpl_string.cpp"
	"upstream.patched/port/cpl_strtod.cpp"
	"upstream.patched/port/cpl_time.cpp"
	"upstream.patched/port/cpl_vsi_mem.cpp"
	"upstream.patched/port/cpl_vsil.cpp"
	"upstream.patched/port/cpl_vsil_abstract_archive.cpp"
	"upstream.patched/port/cpl_vsil_buffered_reader.cpp"
	"upstream.patched/port/cpl_vsil_cache.cpp"
	"upstream.patched/port/cpl_vsil_curl.cpp"
	"upstream.patched/port/cpl_vsil_curl_streaming.cpp"
	"upstream.patched/port/cpl_vsil_gzip.cpp"
	"upstream.patched/port/cpl_vsil_sparsefile.cpp"
	"upstream.patched/port/cpl_vsil_stdin.cpp"
	"upstream.patched/port/cpl_vsil_stdout.cpp"
	"upstream.patched/port/cpl_vsil_subfile.cpp"
	"upstream.patched/port/cpl_vsil_tar.cpp"
	"upstream.patched/port/cpl_vsisimple.cpp"
	"upstream.patched/port/cpl_xml_validate.cpp"
	"upstream.patched/port/cplgetsymbol.cpp"
	"upstream.patched/port/cplkeywordparser.cpp"
	"upstream.patched/port/cplstring.cpp"
	"upstream.patched/port/cplstringlist.cpp"
	${target_specific_sources}
)

add_dependencies(gdal_static
	z_static
)

target_compile_definitions(gdal_static
	PRIVATE
		-DHAVE_LIBZ=1
		-DFRMT_gtiff
		${target_specific_private_definitions}
	PUBLIC
		-DCPL_DLL=
)
target_include_directories(gdal_static
	PRIVATE
		"upstream.patched/frmts/gtiff"
		"upstream.patched/frmts/gtiff/libtiff"
		"upstream.patched/frmts/gtiff/libgeotiff"
		"upstream.patched/ogr/ogrsf_frmts"
		"upstream.patched/ogr/ogrsf_frmts/mem"
	PUBLIC
		"upstream.patched/gcore"
		"upstream.patched/ogr"
		"upstream.patched/alg"
		"upstream.patched/port"
		"${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(gdal_static
	LINK_PUBLIC
		z_static
		${target_specific_public_libraries}
)
