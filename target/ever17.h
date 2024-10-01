#pragma once

#include "base.h"
#include "common.h"
#include "hook_manager.h"

#include <boost/interprocess/mapped_region.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>
#include <type_traits>

namespace targets {
namespace kid {
class ever17 {
  std::string m_prev;

  boost::interprocess::mapped_region m_region;

public:
  struct textoutw_hook
      : hooks::hook_base<decltype(&TextOutA), HDC, int, int, LPCSTR, int> {
    using hook_base::hook_base;
    using return_t = hook_base::return_t;

    static return_t WINAPI fake_call(HDC hdc, int x, int y,
                                     const char *lpString, int c);
  };

  struct bitblt_hook : hooks::hook_base<decltype(&BitBlt), HDC, int, int, int,
                                        int, HDC, int, int, DWORD> {
    using hook_base::hook_base;
    using return_t = hook_base::return_t;

    static return_t WINAPI fake_call(HDC hdc, int x, int y, int cx, int cy,
                                     HDC hdcSrc, int x1, int y1, DWORD rop);
  };

  common::shared_memory *get_shm_ptr();

  std::atomic<HWND> m_main_wnd = NULL;
  boost::unordered::concurrent_flat_set<HDC> m_registered_text_dcs;

  static std::shared_ptr<ever17>
  try_create(hook_manager &hm, boost::interprocess::mapped_region &&region);

  ~ever17();

private:
  textoutw_hook m_textoutw_hook{TextOutA};
  bitblt_hook m_bitblt_hook{BitBlt};
};
} // namespace kid
} // namespace targets
