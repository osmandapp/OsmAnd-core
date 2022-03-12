project(gdal)

prepare_upstream()

set(target_specific_sources "")
set(target_specific_public_libraries "")
set(target_specific_private_definitions "")

if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
	configure_file("upstream.patched/port/cpl_config.h.vc.common" "${CMAKE_CURRENT_BINARY_DIR}/cpl_config.h" COPYONLY)
else()
	include("cpl_config.cmake")
endif()

if (WORDS_BIGENDIAN)
	set(HOST_FILLORDER "FILLORDER_MSB2LSB")
else()
	set(HOST_FILLORDER "FILLORDER_LSB2MSB")
endif()

if (CMAKE_TARGET_OS STREQUAL "android")
	# NOTE: See https://android.googlesource.com/platform/bionic/+/master/docs/32-bit-abi.md
	if (NOT UNIX_STDIO_64)
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-DBUILD_WITHOUT_64BIT_OFFSET
		)
	endif()
endif()

file(GLOB gdal_alg "upstream.patched/gdal/alg/*.c*")

file(GLOB gdal_apps "upstream.patched/gdal/apps/*_lib.c*")
set(gdal_apps_common
	"upstream.patched/gdal/apps/commonutils.cpp"
)

file(GLOB gdal_frmts "upstream.patched/gdal/frmts/*.c*")
file(GLOB gdal_frmts_derived "upstream.patched/gdal/frmts/derived/*.c*")
file(GLOB gdal_frmts_gtiff "upstream.patched/gdal/frmts/gtiff/*.c*")
file(GLOB gdal_frmts_gtiff__remove
	"upstream.patched/gdal/frmts/gtiff/*jxl.c*"
)
list(REMOVE_ITEM gdal_frmts_gtiff ${gdal_frmts_gtiff__remove})

file(GLOB gdal_frmts_gtiff_libtiff "upstream.patched/gdal/frmts/gtiff/libtiff/*.c*")
file(GLOB gdal_frmts_gtiff_libgeotiff "upstream.patched/gdal/frmts/gtiff/libgeotiff/*.c*")
file(GLOB gdal_frmts_hfa "upstream.patched/gdal/frmts/hfa/*.c*")
file(GLOB gdal_frmts_kml "upstream.patched/gdal/frmts/kml/*.c*")
file(GLOB gdal_frmts_mem "upstream.patched/gdal/frmts/mem/*.c*")
file(GLOB gdal_frmts_png "upstream.patched/gdal/frmts/png/*.c*")
file(GLOB gdal_frmts_vrt "upstream.patched/gdal/frmts/vrt/*.c*")

# TODO: fizzers

file(GLOB gdal_gcore "upstream.patched/gdal/gcore/*.c*")
file(GLOB gdal_gcore_mdreader "upstream.patched/gdal/gcore/mdreader/*.c*")

# TODO: gnm

file(GLOB gdal_ogr "upstream.patched/gdal/ogr/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts "upstream.patched/gdal/ogr/ogrsf_frmts/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_generic "upstream.patched/gdal/ogr/ogrsf_frmts/generic/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_geojson "upstream.patched/gdal/ogr/ogrsf_frmts/geojson/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_geojson_libjson "upstream.patched/gdal/ogr/ogrsf_frmts/geojson/libjson/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_kml "upstream.patched/gdal/ogr/ogrsf_frmts/kml/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_mem "upstream.patched/gdal/ogr/ogrsf_frmts/mem/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_mitab "upstream.patched/gdal/ogr/ogrsf_frmts/mitab/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_vrt "upstream.patched/gdal/ogr/ogrsf_frmts/vrt/*.c*")
file(GLOB gdal_ogr_ogrfs_frmts_shape "upstream.patched/gdal/ogr/ogrsf_frmts/shape/*.c*")

set(gdal_port
	"upstream.patched/gdal/port/cpl_alibaba_oss.cpp"
	"upstream.patched/gdal/port/cpl_atomic_ops.cpp"
	"upstream.patched/gdal/port/cpl_aws.cpp"
	"upstream.patched/gdal/port/cpl_aws_win32.cpp"
	"upstream.patched/gdal/port/cpl_azure.cpp"
	"upstream.patched/gdal/port/cpl_base64.cpp"
	"upstream.patched/gdal/port/cpl_compressor.cpp"
	"upstream.patched/gdal/port/cpl_conv.cpp"
	"upstream.patched/gdal/port/cpl_cpu_features.cpp"
	"upstream.patched/gdal/port/cpl_csv.cpp"
	"upstream.patched/gdal/port/cpl_error.cpp"
	"upstream.patched/gdal/port/cpl_findfile.cpp"
	"upstream.patched/gdal/port/cpl_getexecpath.cpp"
	"upstream.patched/gdal/port/cpl_google_cloud.cpp"
	"upstream.patched/gdal/port/cpl_google_oauth2.cpp"
	"upstream.patched/gdal/port/cpl_hash_set.cpp"
	"upstream.patched/gdal/port/cpl_http.cpp"
	"upstream.patched/gdal/port/cpl_json.cpp"
	"upstream.patched/gdal/port/cpl_json_streaming_parser.cpp"
	"upstream.patched/gdal/port/cpl_json_streaming_writer.cpp"
	"upstream.patched/gdal/port/cpl_list.cpp"
	"upstream.patched/gdal/port/cpl_md5.cpp"
	"upstream.patched/gdal/port/cpl_minixml.cpp"
	"upstream.patched/gdal/port/cpl_minizip_ioapi.cpp"
	"upstream.patched/gdal/port/cpl_minizip_unzip.cpp"
	"upstream.patched/gdal/port/cpl_minizip_zip.cpp"
	"upstream.patched/gdal/port/cpl_multiproc.cpp"
	"upstream.patched/gdal/port/cpl_path.cpp"
	"upstream.patched/gdal/port/cpl_progress.cpp"
	"upstream.patched/gdal/port/cpl_quad_tree.cpp"
	"upstream.patched/gdal/port/cpl_recode.cpp"
	"upstream.patched/gdal/port/cpl_recode_iconv.cpp"
	"upstream.patched/gdal/port/cpl_recode_stub.cpp"
	"upstream.patched/gdal/port/cpl_sha1.cpp"
	"upstream.patched/gdal/port/cpl_spawn.cpp"
	"upstream.patched/gdal/port/cpl_string.cpp"
	"upstream.patched/gdal/port/cpl_strtod.cpp"
	"upstream.patched/gdal/port/cpl_swift.cpp"
	"upstream.patched/gdal/port/cpl_time.cpp"
	"upstream.patched/gdal/port/cpl_userfaultfd.cpp"
	"upstream.patched/gdal/port/cpl_vax.cpp"
	"upstream.patched/gdal/port/cpl_virtualmem.cpp"
	"upstream.patched/gdal/port/cpl_vsi_error.cpp"
	"upstream.patched/gdal/port/cpl_vsi_mem.cpp"
	"upstream.patched/gdal/port/cpl_vsil.cpp"
	"upstream.patched/gdal/port/cpl_vsil_abstract_archive.cpp"
	"upstream.patched/gdal/port/cpl_vsil_adls.cpp"
	"upstream.patched/gdal/port/cpl_vsil_az.cpp"
	"upstream.patched/gdal/port/cpl_vsil_buffered_reader.cpp"
	"upstream.patched/gdal/port/cpl_vsil_cache.cpp"
	"upstream.patched/gdal/port/cpl_vsil_crypt.cpp"
	"upstream.patched/gdal/port/cpl_vsil_curl.cpp"
	"upstream.patched/gdal/port/cpl_vsil_curl_streaming.cpp"
	"upstream.patched/gdal/port/cpl_vsil_gs.cpp"
	"upstream.patched/gdal/port/cpl_vsil_gzip.cpp"
	"upstream.patched/gdal/port/cpl_vsil_hdfs.cpp"
	"upstream.patched/gdal/port/cpl_vsil_oss.cpp"
	"upstream.patched/gdal/port/cpl_vsil_plugin.cpp"
	"upstream.patched/gdal/port/cpl_vsil_s3.cpp"
	"upstream.patched/gdal/port/cpl_vsil_sparsefile.cpp"
	"upstream.patched/gdal/port/cpl_vsil_stdin.cpp"
	"upstream.patched/gdal/port/cpl_vsil_stdout.cpp"
	"upstream.patched/gdal/port/cpl_vsil_subfile.cpp"
	"upstream.patched/gdal/port/cpl_vsil_swift.cpp"
	"upstream.patched/gdal/port/cpl_vsil_tar.cpp"
	"upstream.patched/gdal/port/cpl_vsil_unix_stdio_64.cpp"
	"upstream.patched/gdal/port/cpl_vsil_uploadonclose.cpp"
	"upstream.patched/gdal/port/cpl_vsil_webhdfs.cpp"
	"upstream.patched/gdal/port/cpl_vsil_win32.cpp"
	"upstream.patched/gdal/port/cpl_vsisimple.cpp"
	"upstream.patched/gdal/port/cpl_worker_thread_pool.cpp"
	"upstream.patched/gdal/port/cpl_xml_validate.cpp"
	"upstream.patched/gdal/port/cplgetsymbol.cpp"
	"upstream.patched/gdal/port/cplkeywordparser.cpp"
	"upstream.patched/gdal/port/cplstring.cpp"
	"upstream.patched/gdal/port/cplstringlist.cpp"
)

add_library(gdal_static STATIC
	${gdal_alg}
	${gdal_apps}
	${gdal_apps_common}
	${gdal_frmts}
	${gdal_frmts_derived}
	${gdal_frmts_gtiff}
	${gdal_frmts_gtiff_libtiff}
	${gdal_frmts_gtiff_libgeotiff}
	${gdal_frmts_hfa}
	${gdal_frmts_kml}
	${gdal_frmts_mem}
	${gdal_frmts_png}
	${gdal_frmts_vrt}
	${gdal_gcore}
	${gdal_gcore_mdreader}
	${gdal_ogr}
	${gdal_ogr_ogrfs_frmts}
	${gdal_ogr_ogrfs_frmts_generic}
	${gdal_ogr_ogrfs_frmts_geojson}
	${gdal_ogr_ogrfs_frmts_geojson_libjson}
	${gdal_ogr_ogrfs_frmts_kml}
	${gdal_ogr_ogrfs_frmts_mem}
	${gdal_ogr_ogrfs_frmts_mitab}
	${gdal_ogr_ogrfs_frmts_vrt}
	${gdal_ogr_ogrfs_frmts_shape}
	${gdal_port}
	${target_specific_sources}
)

target_compile_definitions(gdal_static
	PRIVATE
		-DGDAL_COMPILATION
		-DHOST_FILLORDER=${HOST_FILLORDER}
		-DHAVE_LIBZ=1
		-DHAVE_EXPAT=1
		-DFRMT_derived
		-DFRMT_gtiff
		-DFRMT_hfa
		-DFRMT_kml
		-DFRMT_mem
		-DFRMT_png
		-DFRMT_vrt
		-DSHAPE_ENABLED=1
		${target_specific_private_definitions}
	PUBLIC
		-DCPL_DLL=
		-DCPL_INTERNAL=
		-DCPL_UNSTABLE_API=
)
target_include_directories(gdal_static
	PUBLIC
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${CMAKE_CURRENT_LIST_DIR}"
		"upstream.patched/gdal/alg"
		"upstream.patched/gdal/alg/marching_squares"
		"upstream.patched/gdal/apps"
		"upstream.patched/gdal/frmts"
		"upstream.patched/gdal/frmts/gtiff"
		"upstream.patched/gdal/frmts/gtiff/libtiff"
		"upstream.patched/gdal/frmts/gtiff/libgeotiff"
		"upstream.patched/gdal/frmts/mem"
		"upstream.patched/gdal/frmts/png"
		"upstream.patched/gdal/frmts/vrt"
		"upstream.patched/gdal/gcore"
		"upstream.patched/gdal/gcore/mdreader"
		"upstream.patched/gdal/ogr"
		"upstream.patched/gdal/ogr/ogrsf_frmts"
		"upstream.patched/gdal/ogr/ogrsf_frmts/generic"
		"upstream.patched/gdal/ogr/ogrsf_frmts/geojson"
		"upstream.patched/gdal/ogr/ogrsf_frmts/geojson/libjson"
		"upstream.patched/gdal/ogr/ogrsf_frmts/mem"
		"upstream.patched/gdal/ogr/ogrsf_frmts/vrt"
		"upstream.patched/gdal/ogr/ogrsf_frmts/shape"
		"upstream.patched/gdal/port"
)
target_link_libraries(gdal_static
	LINK_PUBLIC
		z_static
		png_static
		jpeg_static
		expat_static
		proj_static
		${target_specific_public_libraries}
)
