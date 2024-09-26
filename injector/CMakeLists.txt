add_executable(injector "injector.cpp" "process_info.cpp" "server.cpp")

target_link_libraries(injector PRIVATE common Boost::boost Boost::log Boost::log_setup Boost::thread Boost::system -static-libgcc -static-libstdc++ -static) 

