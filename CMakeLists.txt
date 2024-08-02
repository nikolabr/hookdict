# CMakeList.txt : 最上位の CMake プロジェクト ファイル。グローバル構成を行います
# また、サブプロジェクトをここに含めます。
#
cmake_minimum_required (VERSION 3.8)

# サポートされている場合は、MSVC コンパイラのホット リロードを有効にします。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("hookdict")

include(FetchContent)
include(FindPkgConfig)

set(WIL_BUILD_TESTS OFF)
set(VCPKG_APPLOCAL_DEPS OFF)

FetchContent_Declare(
	wil
	GIT_REPOSITORY https://github.com/microsoft/wil
	GIT_TAG 80ea92c7ab42bf50118c557065bd2db6d8b48f50
)

FetchContent_MakeAvailable(wil)

find_package(PkgConfig REQUIRED)
find_package(minhook CONFIG REQUIRED)
pkg_check_modules(ICU REQUIRED icu-i18n)
# find_package(FFTW3 CONFIG REQUIRED)

# windows.h max collides with std::max in fftwpp
# add_definitions(/DNOMINMAX /D_WINSOCKAPI_)
add_definitions(/DNOMINMAX)

# サブプロジェクトを含めます。
add_subdirectory("common")
add_subdirectory("fastcorr")
add_subdirectory ("injector")
add_subdirectory("target")
