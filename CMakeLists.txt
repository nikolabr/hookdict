cmake_minimum_required(VERSION 3.20)

project("hookdict")

find_package(Boost)
find_package(minhook)
find_package(rpclib)

add_subdirectory("common")
add_subdirectory("fastcorr")
add_subdirectory("injector")
add_subdirectory("target")