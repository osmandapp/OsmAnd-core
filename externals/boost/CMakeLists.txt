project(boost)

prepare_upstream()
build_upstream()

add_library(boost_static_precompiled INTERFACE)
target_include_directories(boost_static_precompiled
	INTERFACE
		"upstream.patched"
)

if (CMAKE_TARGET_OS STREQUAL "android")
	set(libs_dir "upstream.patched.${CMAKE_TARGET_OS}.${CMAKE_COMPILER_FAMILY}-${CMAKE_TARGET_CPU_ARCH}.static")
	target_link_libraries(boost_static_precompiled
		INTERFACE
			"${libs_dir}/stage/lib/libboost_atomic-gcc-mt-1_56.a"
			"${libs_dir}/stage/lib/libboost_system-gcc-mt-1_56.a"
			"${libs_dir}/stage/lib/libboost_thread-gcc-mt-1_56.a"
	)
endif()
