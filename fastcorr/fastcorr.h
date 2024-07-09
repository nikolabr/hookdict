// fastcorr.h : Header file for your target.

#pragma once

#include <fftw3.h>
#include <fftwpp/Array.h>
#include <fftwpp/fftw++.h>

Array::array2<double> glyph_bitmap_to_array2(uint32_t* ptr, unsigned int x_size, unsigned int y_size);