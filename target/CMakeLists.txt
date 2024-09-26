add_library(hookdict_target SHARED "base.cpp" "ever17.cpp" "hook_manager.cpp" "target.cpp")

target_link_libraries(hookdict_target PRIVATE minhook::minhook common fastcorr -static-libgcc -static-libstdc++ -static)
