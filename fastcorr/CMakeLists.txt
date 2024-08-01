add_library(fastcorr STATIC "fastcorr.cpp" "fastcorr.h")

target_include_directories(fastcorr PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# find_path(FFTWPP_INCLUDE_DIRS "fftwpp/Array.cc")
# target_include_directories(fastcorr PRIVATE ${FFTWPP_INCLUDE_DIRS})
# target_link_libraries(fastcorr PRIVATE FFTW3::fftw3)
