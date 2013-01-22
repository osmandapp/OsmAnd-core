if(CMAKE_COMPILER_FAMILY STREQUAL "gcc")
        set(USE_WHOLE_LIBRARY_ "-Wl,--whole-archive -L${CMAKE_LIBRARY_OUTPUT_DIRECTORY} -l")
        set(_USE_WHOLE_LIBRARY " -Wl,--no-whole-archive")
else()
        set(USE_WHOLE_LIBRARY_ "")
        set(_USE_WHOLE_LIBRARY "")
endif()

