project(geographiclib)

prepare_upstream()

add_library(GeographicLib_STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/CircularEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/DMS.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/MGRS.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/SphericalEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/Utility.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/GeoCoords.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/Math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/PolarStereographic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/TransverseMercator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/src/UTMUPS.cpp)

set(GeographicLib_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/upstream.patched/include)

target_include_directories(GeographicLib_STATIC PUBLIC ${GeographicLib_INCLUDE_DIR})
