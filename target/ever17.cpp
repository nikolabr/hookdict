#include "ever17.h"
#include "base.h"
#include "msg.h"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <string>

#include <boost/interprocess/mapped_region.hpp>
#include <wingdi.h>

using namespace targets::kid;

static std::shared_ptr<ever17> g_ever17 = nullptr;

common::shared_memory *targets::kid::ever17::get_shm_ptr() {
  void *p = m_region.get_address();
  return static_cast<common::shared_memory *>(p);
}

// LRESULT WINAPI wnd_hook(int code, WPARAM wp, LPARAM lp) {
//   auto shm_ptr = g_ever17->get_shm_ptr();

//   PCWPRETSTRUCT ps = reinterpret_cast<PCWPRETSTRUCT>(lp);
//   send_message(shm_ptr, common::target_wnd_hook_message_t {
//       .m_msg{ps->message}
//       });

//   return 0;
// }

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

  auto *shm_ptr = ptr->get_shm_ptr();
  send_message(shm_ptr,
               common::target_connected_message_t{.m_window_name{
                   "Ever17 - the out of infinity -  Premium Edition DVD"}});

  ptr->m_main_wnd = ::FindWindow(
      nullptr, "Ever17 - the out of infinity -  Premium Edition DVD");
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

  int res = g_ever17->m_textoutw_hook.m_old_ptr(hdc, x, y, lpString, c);

  if (g_ever17->m_main_wnd != NULL) {
    HDC hdc = ::GetDC(g_ever17->m_main_wnd);

    int width = ::GetDeviceCaps(hdc, HORZRES);
    int height = ::GetDeviceCaps(hdc, VERTRES);

    HDC hCaptureDC = ::CreateCompatibleDC(hdc);
    HBITMAP hBitmap = ::CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ hOld = ::SelectObject(hCaptureDC, hBitmap);
    BOOL bOK = ::BitBlt(hCaptureDC, 0, 0, width, height, hdc, 0, 0,
                        SRCCOPY | CAPTUREBLT);

    ::SelectObject(hCaptureDC, hOld);
    ::DeleteDC(hCaptureDC);

    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);

    ::GetDIBits(hdc, hBitmap, 0, 0, NULL, &bi, DIB_RGB_COLORS);

    bi.bmiHeader.biCompression = BI_RGB;

    DWORD lines = ::GetDIBits(hdc, hBitmap, 0, bi.bmiHeader.biHeight,
                              (LPVOID)shm_ptr->m_img_buf.m_buf.data(), &bi,
                              DIB_RGB_COLORS);

    shm_ptr->m_img_buf.m_sem.wait();
    send_message(shm_ptr, common::bitmap_update_message_t{
                              .m_width{bi.bmiHeader.biWidth},
                              .m_height{bi.bmiHeader.biHeight},
                              .m_lines{lines}});
    shm_ptr->m_img_buf.m_sem.post();

    ::DeleteObject(hBitmap);
    ::ReleaseDC(NULL, hdc);
  }

  return res;
}

ever17::~ever17() {
  auto *shm_ptr = get_shm_ptr();
  send_message(shm_ptr, common::target_close_message_t{});

  g_ever17 = nullptr;
}
