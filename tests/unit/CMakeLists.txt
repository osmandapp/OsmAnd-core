project(unit)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Need to rewrite to not compile things twice
include_directories(../..)
set(SRC_LIST ${SRC_LIST} ../../SearchByCoordinate.cpp ../../SearchByCoordinate.h)

add_executable(TestSearchByCoordinate ${SRC_LIST})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOMOC_RELAXED_MODE ON)
set(QT_MOC_EXECUTABLE moc-qt5)
#qt_wrap_cpp(TestSearchByCoordinate TestSearchByCoordinate.cpp TestSearchByCoordinate.moc)
#qt_wrap_cpp(TestSearchByCoordinate TestSearchByCoordinate.moc TestSearchByCoordinate.cpp)

find_package(Qt5Test)

target_link_libraries(TestSearchByCoordinate Qt5::Test)

enable_testing(true)

add_test(TestSearchByCoordinate TestSearchByCoordinate)
