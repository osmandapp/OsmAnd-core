project(gdal)

add_library(gdal_static STATIC
	"upstream.patched/frmts/gdalallregister.cpp"
)

target_compile_definitions(gdal_static
	PRIVATE
		-DFRMT_gtiff
		-DFRMT_srtmhgt
		-DFRMT_usgsdem
	PUBLIC
		-DGLEW_STATIC
)
target_include_directories(gdal_static
	PUBLIC
		"."
		"upstream.patched/gcore"
		"upstream.patched/port"
)
target_link_libraries(gdal_static
	LINK_PUBLIC
)