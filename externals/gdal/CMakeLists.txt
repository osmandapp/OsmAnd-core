project(gdal)

set(target_specific_sources "")
set(target_specific_public_libraries "")
if(CMAKE_TARGET_OS STREQUAL "linux")
elseif(CMAKE_TARGET_OS STREQUAL "darwin")
elseif(CMAKE_TARGET_OS STREQUAL "windows")
	set(target_specific_sources ${target_specific_sources}
		"upstream.patched/port/cpl_vsil_win32.cpp"
	)
	set(target_specific_public_libraries ${target_specific_public_libraries}
		Ws2_32
	)
endif()

file(GLOB_RECURSE gdal_gcore_sources "upstream.patched/gcore/*.cpp")
file(GLOB_RECURSE gdal_alg_sources "upstream.patched/alg/*.cpp")
file(GLOB gdal_ogr_sources "upstream.patched/ogr/*.cpp")
file(GLOB_RECURSE gdal_ogr_frmts_mem_sources "upstream.patched/ogr/frmts/mem/*.cpp")
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
		-DFRMT_gtiff
	PUBLIC
)
target_include_directories(gdal_static
	PRIVATE
		"upstream.patched/frmts/gtiff"
		"upstream.patched/frmts/gtiff/libtiff"
		"upstream.patched/frmts/gtiff/libgeotiff"
		"upstream.patched/ogr/ogrsf_frmts"
		"upstream.patched/ogr/ogrsf_frmts/mem"
	PUBLIC
		"."
		"upstream.patched/gcore"
		"upstream.patched/ogr"
		"upstream.patched/alg"
		"upstream.patched/port"
)
target_link_libraries(gdal_static
	LINK_PUBLIC
		z_static
		${target_specific_public_libraries}
)