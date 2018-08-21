project(jpeg_osmand)

set(ROOT "${OSMAND_ROOT}/externals/skia/upstream.patched/third_party/externals/libjpeg-turbo")
set(UPSTREAM "${ROOT}")
#include_directories(AFTER SYSTEM
#	"${ROOT}"
#)
add_definitions(-DNO_GETENV)
if(CMAKE_COMPILER_FAMILY STREQUAL "gcc" OR CMAKE_COMPILER_FAMILY STREQUAL "clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

include(CheckTypeSize)
check_type_size(size_t SIZE_T)

###############################################################################
# CONFIGURATION OPTIONS
###############################################################################

macro(boolean_number var)
  if(${var})
    set(${var} 1)
  else()
    set(${var} 0)
  endif()
endmacro()

option(ENABLE_SHARED "Build shared libraries" TRUE)
boolean_number(ENABLE_SHARED)
option(ENABLE_STATIC "Build static libraries" TRUE)
boolean_number(ENABLE_STATIC)
option(REQUIRE_SIMD "Generate a fatal error if SIMD extensions are not available for this platform (default is to fall back to a non-SIMD build)" FALSE)
boolean_number(REQUIRE_SIMD)
option(WITH_12BIT "Encode/decode JPEG images with 12-bit samples (implies WITH_ARITH_DEC=0 WITH_ARITH_ENC=0 WITH_JAVA=0 WITH_SIMD=0 WITH_TURBOJPEG=0 )" FALSE)
boolean_number(WITH_12BIT)
option(WITH_ARITH_DEC "Include arithmetic decoding support when emulating the libjpeg v6b API/ABI" TRUE)
boolean_number(WITH_ARITH_DEC)
option(WITH_ARITH_ENC "Include arithmetic encoding support when emulating the libjpeg v6b API/ABI" TRUE)
boolean_number(WITH_ARITH_ENC)
option(WITH_JAVA "Build Java wrapper for the TurboJPEG API library (implies ENABLE_SHARED=1)" FALSE)
boolean_number(WITH_JAVA)
option(WITH_JPEG7 "Emulate libjpeg v7 API/ABI (this makes ${CMAKE_PROJECT_NAME} backward-incompatible with libjpeg v6b)" FALSE)
boolean_number(WITH_JPEG7)
option(WITH_JPEG8 "Emulate libjpeg v8 API/ABI (this makes ${CMAKE_PROJECT_NAME} backward-incompatible with libjpeg v6b)" FALSE)
boolean_number(WITH_JPEG8)
option(WITH_MEM_SRCDST "Include in-memory source/destination manager functions when emulating the libjpeg v6b or v7 API/ABI" TRUE)
boolean_number(WITH_MEM_SRCDST)
option(WITH_SIMD "Include SIMD extensions, if available for this platform" TRUE)
boolean_number(WITH_SIMD)
option(WITH_TURBOJPEG "Include the TurboJPEG API library and associated test programs" TRUE)
boolean_number(WITH_TURBOJPEG)

macro(report_option var desc)
  if(${var})
    message(STATUS "${desc} enabled (${var} = ${${var}})")
  else()
    message(STATUS "${desc} disabled (${var} = ${${var}})")
  endif()
endmacro()

if(WITH_JAVA)
  set(ENABLE_SHARED 1)
endif()

# Explicitly setting CMAKE_POSITION_INDEPENDENT_CODE=FALSE disables PIC for all
# targets, which will cause the shared library builds to fail.  Thus, if shared
# libraries are enabled and CMAKE_POSITION_INDEPENDENT_CODE is explicitly set
# to FALSE, we need to unset it, thus restoring the default behavior
# (automatically using PIC for shared library targets.)
if(DEFINED CMAKE_POSITION_INDEPENDENT_CODE AND
  NOT CMAKE_POSITION_INDEPENDENT_CODE AND ENABLE_SHARED)
  unset(CMAKE_POSITION_INDEPENDENT_CODE CACHE)
endif()

report_option(ENABLE_SHARED "Shared libraries")
report_option(ENABLE_STATIC "Static libraries")

if(WITH_12BIT)
  set(WITH_ARITH_DEC 0)
  set(WITH_ARITH_ENC 0)
  set(WITH_JAVA 0)
  set(WITH_SIMD 0)
  set(WITH_TURBOJPEG 0)
  set(BITS_IN_JSAMPLE 12)
else()
  set(BITS_IN_JSAMPLE 8)
endif()
report_option(WITH_12BIT "12-bit JPEG support")

if(WITH_JPEG8 OR WITH_JPEG7)
  set(WITH_ARITH_ENC 1)
  set(WITH_ARITH_DEC 1)
endif()
if(WITH_JPEG8)
  set(WITH_MEM_SRCDST 0)
endif()

if(WITH_ARITH_DEC)
  set(D_ARITH_CODING_SUPPORTED 1)
endif()
if(NOT WITH_12BIT)
  report_option(WITH_ARITH_DEC "Arithmetic decoding support")
endif()

if(WITH_ARITH_ENC)
  set(C_ARITH_CODING_SUPPORTED 1)
endif()
if(NOT WITH_12BIT)
  report_option(WITH_ARITH_ENC "Arithmetic encoding support")
endif()

if(NOT WITH_12BIT)
  report_option(WITH_TURBOJPEG "TurboJPEG API library")
  report_option(WITH_JAVA "TurboJPEG Java wrapper")
endif()

if(WITH_MEM_SRCDST)
  set(MEM_SRCDST_SUPPORTED 1)
  set(MEM_SRCDST_FUNCTIONS "global:  jpeg_mem_dest;  jpeg_mem_src;")
endif()
if(NOT WITH_JPEG8)
  report_option(WITH_MEM_SRCDST "In-memory source/destination managers")
endif()

set(SO_AGE 2)
if(WITH_MEM_SRCDST)
  set(SO_AGE 3)
endif()

if(WITH_JPEG8)
  set(JPEG_LIB_VERSION 80)
elseif(WITH_JPEG7)
  set(JPEG_LIB_VERSION 70)
else()
  set(JPEG_LIB_VERSION 62)
endif()

math(EXPR JPEG_LIB_VERSION_DIV10 "${JPEG_LIB_VERSION} / 10")
math(EXPR JPEG_LIB_VERSION_MOD10 "${JPEG_LIB_VERSION} % 10")
if(JPEG_LIB_VERSION STREQUAL "62")
  set(DEFAULT_SO_MAJOR_VERSION ${JPEG_LIB_VERSION})
else()
  set(DEFAULT_SO_MAJOR_VERSION ${JPEG_LIB_VERSION_DIV10})
endif()
if(JPEG_LIB_VERSION STREQUAL "80")
  set(DEFAULT_SO_MINOR_VERSION 2)
else()
  set(DEFAULT_SO_MINOR_VERSION 0)
endif()

# This causes SO_MAJOR_VERSION/SO_MINOR_VERSION to reset to defaults if
# WITH_JPEG7 or WITH_JPEG8 has changed.
if((DEFINED WITH_JPEG7_INT AND NOT WITH_JPEG7 EQUAL WITH_JPEG7_INT) OR
  (DEFINED WITH_JPEG8_INT AND NOT WITH_JPEG8 EQUAL WITH_JPEG8_INT))
  set(FORCE_SO_VERSION "FORCE")
endif()
set(WITH_JPEG7_INT ${WITH_JPEG7} CACHE INTERNAL "")
set(WITH_JPEG8_INT ${WITH_JPEG8} CACHE INTERNAL "")

set(SO_MAJOR_VERSION ${DEFAULT_SO_MAJOR_VERSION} CACHE STRING
  "Major version of the libjpeg API shared library (default: ${DEFAULT_SO_MAJOR_VERSION})"
  ${FORCE_SO_VERSION})
set(SO_MINOR_VERSION ${DEFAULT_SO_MINOR_VERSION} CACHE STRING
  "Minor version of the libjpeg API shared library (default: ${DEFAULT_SO_MINOR_VERSION})"
  ${FORCE_SO_VERSION})

set(HAVE_UNSIGNED_CHAR 1)

set(JPEG_LIB_VERSION_DECIMAL "${JPEG_LIB_VERSION_DIV10}.${JPEG_LIB_VERSION_MOD10}")
message(STATUS "Emulating libjpeg API/ABI v${JPEG_LIB_VERSION_DECIMAL} (WITH_JPEG7 = ${WITH_JPEG7}, WITH_JPEG8 = ${WITH_JPEG8})")
message(STATUS "libjpeg API shared library version = ${SO_MAJOR_VERSION}.${SO_AGE}.${SO_MINOR_VERSION}")

# Because the TurboJPEG API library uses versioned symbols and changes the
# names of functions whenever they are modified in a backward-incompatible
# manner, it is always backward-ABI-compatible with itself, so the major and
# minor SO versions don't change.  However, we increase the middle number (the
# SO "age") whenever functions are added to the API.
set(TURBOJPEG_SO_MAJOR_VERSION 0)
set(TURBOJPEG_SO_VERSION 0.2.0)

configure_file("${UPSTREAM}/jconfig.h.in" "${UPSTREAM}/jconfig.h")
configure_file("${UPSTREAM}/jconfigint.h.in" "${UPSTREAM}/jconfigint.h")

add_library(jpeg_osmand STATIC
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
      "${UPSTREAM}/jcphuff.c"
      "${UPSTREAM}/jcprepct.c"
      "${UPSTREAM}/jcsample.c"
      "${UPSTREAM}/jdapimin.c"
      "${UPSTREAM}/jdapistd.c"
      "${UPSTREAM}/jdarith.c"
      "${UPSTREAM}/jdcoefct.c"
      "${UPSTREAM}/jdcolor.c"
      "${UPSTREAM}/jddctmgr.c"
      "${UPSTREAM}/jdhuff.c"
      "${UPSTREAM}/jdinput.c"
      "${UPSTREAM}/jdmainct.c"
      "${UPSTREAM}/jdmarker.c"
      "${UPSTREAM}/jdmaster.c"
      "${UPSTREAM}/jdmerge.c"
      "${UPSTREAM}/jdphuff.c"
      "${UPSTREAM}/jdpostct.c"
      "${UPSTREAM}/jdsample.c"
      "${UPSTREAM}/jerror.c"
      "${UPSTREAM}/jfdctflt.c"
      "${UPSTREAM}/jfdctfst.c"
      "${UPSTREAM}/jfdctint.c"
      "${UPSTREAM}/jidctflt.c"
      "${UPSTREAM}/jidctfst.c"
      "${UPSTREAM}/jidctint.c"
      "${UPSTREAM}/jidctred.c"
      "${UPSTREAM}/jmemmgr.c"
      "${UPSTREAM}/jmemnobs.c"
      "${UPSTREAM}/jquant1.c"
      "${UPSTREAM}/jquant2.c"
      "${UPSTREAM}/jutils.c"
      "${UPSTREAM}/jsimd_none.c"
)
