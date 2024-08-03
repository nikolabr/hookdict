# CMakeList.txt : jdb の CMake プロジェクト。ソースを含めて、次を定義します:
# プロジェクト専用ロジックはこちらです。
#

# ソースをこのプロジェクトの実行可能ファイルに追加します。
add_executable (injector "injector.cpp" "injector.h" "injector.exe.manifest" "process_info.h" "process_info.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET injector PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(injector PRIVATE WIL::WIL common)
target_link_libraries(injector PUBLIC PkgConfig::ICU)
# target_link_options(injector PRIVATE "/MANIFEST:NO")

# TODO: テストを追加し、必要な場合は、ターゲットをインストールします。
