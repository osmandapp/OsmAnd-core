# Force zlib to use prefixed function names to avoid collisions with system zlib
add_definitions(-DZ_PREFIX)

# Include original CMake configuration file
include("./upstream.patched/CMakeLists.txt")