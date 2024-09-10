#pragma once

#include "common.h"
#include "hooks.h"

#include "fastcorr.h"

#include <list>
#include <string>
#include <vector>

struct glyph_info {
  unsigned int m_val = 0;
  std::vector<unsigned char> m_buffer;

  unsigned int m_x = 0;
  unsigned int m_y = 0;

  glyph_info(unsigned int char_val, unsigned char *ptr, std::size_t n,
             unsigned int x, unsigned int y);
};

static void write_msg(wchar_t const *msg) {
  ::MessageBoxW(nullptr, msg, L"Target message", MB_OK);
}
