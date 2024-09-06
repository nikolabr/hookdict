#pragma once

#include <windows.h>

#include "common.h"
#include "hooks.h"

#include "fastcorr.h"

#include <format>
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

struct glyph_cache {
  std::list<glyph_info> m_glyphs;
};

extern HANDLE g_pipe;

static void create_pipe() {
  g_pipe = CreateFileW(common::hookdict_pipe_name, GENERIC_WRITE, FILE_SHARE_WRITE,
		       nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

static void write_to_pipe(HANDLE pipe_handle, auto &&msg) {
  const std::string pipemsg(msg);
  BOOL res = ::WriteFile(pipe_handle, pipemsg.c_str(),
                         pipemsg.size(), nullptr, nullptr);
  if (!res) {
    write_msg(L"Could not write to pipe");
  }

}

namespace targets {
template <typename T>
void target_log(const T &target, std::wstring const &wstr) {
  std::wstring msg = std::format(L"target {}: {}", target.s_target_name, wstr);
  pipe_write_string(target.m_hPipe, msg);
}
} // namespace targets
