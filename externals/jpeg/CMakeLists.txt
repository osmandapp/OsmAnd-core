project(jpeg)

prepare_upstream()

add_library(jpeg_static STATIC
	"upstream.patched/jaricom.c"
	"upstream.patched/jcapimin.c"
	"upstream.patched/jcapistd.c"
	"upstream.patched/jcarith.c"
	"upstream.patched/jccoefct.c"
	"upstream.patched/jccolor.c"
	"upstream.patched/jcdctmgr.c"
	"upstream.patched/jchuff.c"
	"upstream.patched/jcinit.c"
	"upstream.patched/jcmainct.c"
	"upstream.patched/jcmarker.c"
	"upstream.patched/jcmaster.c"
	"upstream.patched/jcomapi.c"
	"upstream.patched/jcparam.c"
	"upstream.patched/jcprepct.c"
	"upstream.patched/jcsample.c"
	"upstream.patched/jctrans.c"
	"upstream.patched/jdapimin.c"
	"upstream.patched/jdapistd.c"
	"upstream.patched/jdarith.c"
	"upstream.patched/jdatadst.c"
	"upstream.patched/jdatasrc.c"
	"upstream.patched/jdcoefct.c"
	"upstream.patched/jdcolor.c"
	"upstream.patched/jddctmgr.c"
	"upstream.patched/jdhuff.c"
	"upstream.patched/jdinput.c"
	"upstream.patched/jdmainct.c"
	"upstream.patched/jdmarker.c"
	"upstream.patched/jdmaster.c"
	"upstream.patched/jdmerge.c"
	"upstream.patched/jdpostct.c"
	"upstream.patched/jdsample.c"
	"upstream.patched/jdtrans.c"
	"upstream.patched/jerror.c"
	"upstream.patched/jfdctflt.c"
	"upstream.patched/jfdctfst.c"
	"upstream.patched/jfdctint.c"
	"upstream.patched/jidctflt.c"
	"upstream.patched/jidctfst.c"
	"upstream.patched/jidctint.c"
	"upstream.patched/jmemansi.c"
	"upstream.patched/jmemmgr.c"
	"upstream.patched/jquant1.c"
	"upstream.patched/jquant2.c"
	"upstream.patched/jutils.c"
)

set(target_specific_private_definitions "")
if (CMAKE_TARGET_OS STREQUAL "windows")
	if (CMAKE_COMPILER_FAMILY STREQUAL "msvc")
		set(target_specific_private_definitions ${target_specific_private_definitions}
			-D_CRT_SECURE_NO_WARNINGS
		)
	endif()
endif()

target_compile_definitions(jpeg_static
	PRIVATE
		-DNO_GETENV
		${target_specific_private_definitions}
)
target_include_directories(jpeg_static
	PRIVATE
		"."
	PUBLIC
		"upstream.patched"
)
