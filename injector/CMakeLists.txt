# CMakeList.txt : jdb の CMake プロジェクト。ソースを含めて、次を定義します:
# プロジェクト専用ロジックはこちらです。
#

# ソースをこのプロジェクトの実行可能ファイルに追加します。
add_executable (injector "injector.cpp" "injector.h" "elevation.cpp" "elevation.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET injector PROPERTY CXX_STANDARD 20)
endif()

# TODO: テストを追加し、必要な場合は、ターゲットをインストールします。
