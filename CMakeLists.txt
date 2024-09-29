cmake_minimum_required(VERSION 3.20)

project("hookdict")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Boost)
find_package(minhook)

add_subdirectory("common")
add_subdirectory("fastcorr")
add_subdirectory("injector")
add_subdirectory("target")
