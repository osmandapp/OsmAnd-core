project(jpeg)

set(ROOT "${OSMAND_ROOT}/externals/jpeg")
set(UPSTREAM "${ROOT}/upstream.patched")
include_directories(AFTER SYSTEM
	"${ROOT}"
)
add_definitions(-DNO_GETENV)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()
add_library(jpeg STATIC
    "${UPSTREAM}/jaricom.c"
	"${UPSTREAM}/jcapimin.c"
	"${UPSTREAM}/jcapistd.c"
	"${UPSTREAM}/jcarith.c"
	"${UPSTREAM}/jccoefct.c"
	"${UPSTREAM}/jccolor.c"
	"${UPSTREAM}/jcdctmgr.c"
	"${UPSTREAM}/jchuff.c"
	"${UPSTREAM}/jcinit.c"
	"${UPSTREAM}/jcmainct.c"
	"${UPSTREAM}/jcmarker.c"
	"${UPSTREAM}/jcmaster.c"
	"${UPSTREAM}/jcomapi.c"
	"${UPSTREAM}/jcparam.c"
	"${UPSTREAM}/jcprepct.c"
	"${UPSTREAM}/jcsample.c"
	"${UPSTREAM}/jctrans.c"
	"${UPSTREAM}/jdapimin.c"
	"${UPSTREAM}/jdapistd.c"
	"${UPSTREAM}/jdarith.c"
	"${UPSTREAM}/jdatadst.c"
	"${UPSTREAM}/jdatasrc.c"
	"${UPSTREAM}/jdcoefct.c"
	"${UPSTREAM}/jdcolor.c"
	"${UPSTREAM}/jddctmgr.c"
	"${UPSTREAM}/jdhuff.c"
	"${UPSTREAM}/jdinput.c"
	"${UPSTREAM}/jdmainct.c"
	"${UPSTREAM}/jdmarker.c"
	"${UPSTREAM}/jdmaster.c"
	"${UPSTREAM}/jdmerge.c"
	"${UPSTREAM}/jdpostct.c"
	"${UPSTREAM}/jdsample.c"
	"${UPSTREAM}/jdtrans.c"
	"${UPSTREAM}/jerror.c"
	"${UPSTREAM}/jfdctflt.c"
	"${UPSTREAM}/jfdctfst.c"
	"${UPSTREAM}/jfdctint.c"
	"${UPSTREAM}/jidctflt.c"
	"${UPSTREAM}/jidctfst.c"
	"${UPSTREAM}/jidctint.c"
	"${UPSTREAM}/jmemansi.c"
	"${UPSTREAM}/jmemmgr.c"
	"${UPSTREAM}/jquant1.c"
	"${UPSTREAM}/jquant2.c"
	"${UPSTREAM}/jutils.c"
)