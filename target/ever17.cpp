#include "ever17.h"
#include "base.h"
#include "msg.h"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <gdiplus/gdiplusenums.h>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/interprocess/mapped_region.hpp>
#include <gdiplus.h>
#include <wingdi.h>

using namespace targets::kid;

static std::shared_ptr<ever17> g_ever17 = nullptr;

HRESULT GetEncoderClsid(const std::wstring &format, GUID *pGuid) {
  HRESULT hr = S_OK;
  UINT nEncoders = 0; // number of image encoders
  UINT nSize = 0;     // size of the image encoder array in bytes
  std::vector<BYTE> spData;
  Gdiplus::ImageCodecInfo *pImageCodecInfo = NULL;
  Gdiplus::Status status;
  bool found = false;
  if (format.empty() || !pGuid) {
    hr = E_INVALIDARG;
  }
  if (SUCCEEDED(hr)) {
    *pGuid = GUID_NULL;
    status = Gdiplus::GetImageEncodersSize(&nEncoders, &nSize);
    if ((status != Gdiplus::Ok) || (nSize == 0)) {
      hr = E_FAIL;
    }
  }
  if (SUCCEEDED(hr)) {
    spData.resize(nSize);
    pImageCodecInfo = (Gdiplus::ImageCodecInfo *)&spData.front();
    status = Gdiplus::GetImageEncoders(nEncoders, nSize, pImageCodecInfo);
    if (status != Gdiplus::Ok) {
      hr = E_FAIL;
    }
  }
  if (SUCCEEDED(hr)) {
    for (UINT j = 0; j < nEncoders && !found; j++) {
      if (pImageCodecInfo[j].MimeType == format) {
        *pGuid = pImageCodecInfo[j].Clsid;
        found = true;
      }
    }
    hr = found ? S_OK : E_FAIL;
  }
  return hr;
}

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

  std::string dc_text;

  RECT rect;
  ::GetClientRect(g_ever17->m_main_wnd, &rect);

  DWORD width = rect.right - rect.left;
  DWORD height = rect.bottom - rect.top;

  /*
    >You are responsible for deleting the GDI bitmap and the GDI
    palette. However, you should not delete the GDI bitmap or the GDI
    palette until after the GDI+ Bitmap::Bitmap object is deleted or
    goes out of scope.
  */
  HBITMAP hbm = ::CreateCompatibleBitmap(hdc, width, height);

  Gdiplus::Bitmap *bmp = new Gdiplus::Bitmap(hbm, 0);
  Gdiplus::Graphics bmpGraphics(bmp);

  HDC hBmpDC = bmpGraphics.GetHDC();
  DWORD lines = g_ever17->m_bitblt_hook.m_old_ptr(hBmpDC, 0, 0, width, height,
                                                  hdc, 0, 0, SRCCOPY);

  CLSID clsid;
  GetEncoderClsid(L"image/bmp", &clsid);

  IStream *stream = get_img_buf_stream(shm_ptr);
  if (stream == NULL) {
    throw std::runtime_error("Failed to create stream");
  }
  {
    using namespace boost::interprocess;
    shm_ptr->m_img_buf.m_sem.wait();

    auto st = bmp->Save(stream, &clsid, nullptr);
    if (st != Gdiplus::Status::Ok) {
      throw std::runtime_error("Failed to save bitmap to stream");
    }
    shm_ptr->m_img_buf.m_sem.post();
  }

  ::ReleaseDC(NULL, hBmpDC);

  delete bmp;
  stream->Release();
  ::DeleteObject(hbm);

  return res;
}

ever17::~ever17() {
  auto *shm_ptr = get_shm_ptr();
  send_message(shm_ptr, common::target_close_message_t{});

  g_ever17 = nullptr;
}
