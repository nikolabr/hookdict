add_library(hookdict_target
	SHARED 
	"target.cpp"
	"target.h" "base.h" "base.cpp" "hook_manager.h" "hook_manager.cpp" "reallive.h" "reallive.cpp" "ever17.h" "ever17.cpp"
)

target_link_libraries(hookdict_target PRIVATE common)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_target_properties(hookdict_target PROPERTIES CXX_STANDARD 20)
endif()

target_link_libraries(hookdict_target PRIVATE WIL::WIL fastcorr)

target_link_libraries(hookdict_target PRIVATE minhook::minhook)
