project(boost)

prepare_upstream()
build_upstream()

add_library(boost_static_precompiled INTERFACE)
target_include_directories(boost_static_precompiled
	INTERFACE
		"upstream.patched"
)

if (CMAKE_TARGET_OS STREQUAL "android")
	set(libs_dir "${CMAKE_CURRENT_LIST_DIR}/upstream.patched.${CMAKE_TARGET_OS}.${CMAKE_COMPILER_FAMILY}-${CMAKE_TARGET_CPU_ARCH}.static/stage/lib")
	target_link_libraries(boost_static_precompiled
		INTERFACE
			"${libs_dir}/libboost_atomic.a"
			"${libs_dir}/libboost_system.a"
			"${libs_dir}/libboost_thread.a"
			"${libs_dir}/libboost_chrono.a"
	)
endif()
