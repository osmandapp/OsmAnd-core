# SWIG is required for all wrappers
set(OSMAND_SWIG_AVAILABLE OFF)
if (NOT DEFINED ENV{SWIG})
    find_package(SWIG)
    if (SWIG_FOUND)
        set(OSMAND_SWIG_AVAILABLE ON)
        set(SWIG "$SWIG_EXECUTABLE")
    endif()
else()
    set(OSMAND_SWIG_AVAILABLE ON)
    set(SWIG "$ENV{SWIG}")
endif()

# add_generate_swig_target: macro that adds a special target that calls generate.sh
macro(add_generate_swig_target TARGET_NAME)
    if (CMAKE_HOST_WIN32 AND NOT CYGWIN)
        add_custom_target(${TARGET_NAME} bash --login "${CMAKE_CURRENT_LIST_DIR}/generate.sh" "${CMAKE_CURRENT_BINARY_DIR}"
            DEPENDS
                "${CMAKE_CURRENT_LIST_DIR}/generate.sh"
                ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating wrapper sources in '${CMAKE_CURRENT_BINARY_DIR}' using '${CMAKE_CURRENT_LIST_DIR}/generate.sh'...")
    else()
        add_custom_target(${TARGET_NAME} "${CMAKE_CURRENT_LIST_DIR}/generate.sh" "${CMAKE_CURRENT_BINARY_DIR}"
            DEPENDS
                "${CMAKE_CURRENT_LIST_DIR}/generate.sh"
                ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating wrapper sources in '${CMAKE_CURRENT_BINARY_DIR}' using '${CMAKE_CURRENT_LIST_DIR}/generate.sh'...")
    endif()
endmacro()

# Java
set(OSMAND_JAVA_AVAILABLE OFF)
if (CMAKE_TARGET_OS STREQUAL "android")
    set(OSMAND_JAVA_AVAILABLE ON)
elseif (CMAKE_TARGET_OS STREQUAL "ios")
    set(OSMAND_JAVA_AVAILABLE OFF)
else()
    find_package(Java COMPONENTS Development)
    find_package(JNI)

    if (Java_Development_FOUND AND JNI_FOUND)
        set(OSMAND_JAVA_AVAILABLE ON)
    endif ()
endif()
if (OSMAND_JAVA_AVAILABLE AND OSMAND_SWIG_AVAILABLE)
    set(CMAKE_JAVA_TARGET_OUTPUT_DIR "${OSMAND_OUTPUT_ROOT}")
    add_subdirectory("${OSMAND_ROOT}/core/wrappers/java" "core/wrappers/java")
    if (NOT CMAKE_TARGET_OS STREQUAL "android")
        add_subdirectory("${OSMAND_ROOT}/core/samples/java" "core/samples/java")
    endif()
else()
    message("Java or JNI or SWIG was not found, Core wrappers for Java are not enabled")
endif()
