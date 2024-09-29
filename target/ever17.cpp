#include "ever17.h"
#include "base.h"
#include "msg.h"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <gdiplus/gdiplusenums.h>
#include <memory>
#include <string>

#include <boost/interprocess/mapped_region.hpp>
#include <gdiplus.h>
#include <wingdi.h>

using namespace targets::kid;

static std::shared_ptr<ever17> g_ever17 = nullptr;

static std::atomic<unsigned int> counter = 0;

common::shared_memory *targets::kid::ever17::get_shm_ptr() {
  void *p = m_region.get_address();
  return static_cast<common::shared_memory *>(p);
}

std::shared_ptr<ever17>
targets::kid::ever17::try_create(hook_manager &hm,
                                 boost::interprocess::mapped_region &&region) {
  std::filesystem::path module_path = common::get_module_file_name_w();

  auto filename = std::move(module_path).filename().wstring();
  std::transform(filename.begin(), filename.end(), filename.begin(), towupper);

  if (filename != L"EVER17PC.EXE") {
    return nullptr;
  }

  auto ptr = std::make_shared<ever17>();
  g_ever17 = ptr;

  ptr->m_region = std::move(region);
  ptr->m_textoutw_hook.enable(hm, textoutw_hook::fake_call);
  ptr->m_bitblt_hook.enable(hm, bitblt_hook::fake_call);

  auto *shm_ptr = ptr->get_shm_ptr();
  const char *wnd_title = "Ever17 - the out of infinity -  Premium Edition DVD";

  send_message(shm_ptr, common::target_connected_message_t{
                            .m_window_name{std::string(wnd_title)}});

  ptr->m_main_wnd = ::FindWindow(nullptr, wnd_title);
  if (ptr->m_main_wnd == NULL) {
    throw std::runtime_error("Could not find main window!");
  }

  return ptr;
}

ever17::textoutw_hook::return_t WINAPI ever17::textoutw_hook::fake_call(
    HDC hdc, int x, int y, LPCSTR lpString, int c) {
  std::string out(lpString);
  uint32_t codepage = GetACP();

  auto *shm_ptr = g_ever17->get_shm_ptr();
  send_message(shm_ptr, common::target_generic_message_t{.m_text{lpString},
                                                         .m_cp{codepage}});

  return g_ever17->m_textoutw_hook.m_old_ptr(hdc, x, y, lpString, c);
}

ever17::bitblt_hook::return_t
ever17::bitblt_hook::fake_call(HDC hdc, int x, int y, int cx, int cy,
                               HDC hdcSrc, int x1, int y1, DWORD rop) {

  auto res =
      g_ever17->m_bitblt_hook.m_old_ptr(hdc, x, y, cx, cy, hdcSrc, x1, y1, rop);
  auto *shm_ptr = g_ever17->get_shm_ptr();

  RECT rect;
  ::GetClientRect(g_ever17->m_main_wnd, &rect);

  DWORD width = rect.right - rect.left;
  DWORD height = rect.bottom - rect.top;
  
  Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap(::CreateCompatibleBitmap(hdc, width, height), 0);
  Gdiplus::Graphics bmpGraphics(bmp);

  HDC hBmpDC = bmpGraphics.GetHDC();
  DWORD lines = g_ever17->m_bitblt_hook.m_old_ptr(hBmpDC, 0, 0, width, height,
                                                  hdc, 0, 0, SRCCOPY);

  CLSID clsid;
  ::CLSIDFromString(L"{557cf400-1a04-11d3-9a73-0000f81ef32e}", &clsid);
  bmp->Save(L"image.bmp", &clsid, nullptr);

  send_message(shm_ptr, common::bitmap_update_message_t{
      .m_width{(long)width},
      .m_height{(long)height},
      .m_lines{lines}
    });

  // ::ReleaseDC(NULL, hBmpDC);
  delete bmp;
  
  return res;
}

ever17::~ever17() {
  auto *shm_ptr = get_shm_ptr();
  send_message(shm_ptr, common::target_close_message_t{});

  g_ever17 = nullptr;
}
