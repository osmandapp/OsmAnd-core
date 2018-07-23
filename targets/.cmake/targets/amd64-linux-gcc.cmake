set(CMAKE_TARGET_OS linux)
set(CMAKE_TARGET_CPU_ARCH amd64)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_COMPILER_FAMILY gcc)
set(CMAKE_C_COMPILER_FLAGS "-m64 -fstack-protector-all -Wstack-protector")
set(CMAKE_CXX_COMPILER_FLAGS "-std=c++0x -m64 -fstack-protector-all -Wstack-protector")
 
