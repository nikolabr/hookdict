add_executable(injector "injector.cpp" "process_info.cpp")

target_link_libraries(injector PRIVATE common Boost::boost rpclib::rpc)