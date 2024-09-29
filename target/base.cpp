#include "base.h"

#include "msg.h"

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <gdiplus.h>

#include <fstream>
#include <wingdi.h>

outcome::result<std::vector<char>> write_dc_to_bitmap(HDC hdc, DWORD default_size) {
  HDC memdc = ::CreateCompatibleDC(hdc);
  if (memdc == NULL) {
    return outcome::failure(std::error_code());
  }
  
  HBITMAP hbm = ::CreateCompatibleBitmap(hdc, 128, 128);
  if (hbm == NULL) {
    return outcome::failure(std::error_code());
  }
  ::SelectObject(memdc, hbm);
  
  bool bRes = ::BitBlt(memdc, 0, 0, 128, 128, hdc, 0, 0, SRCCOPY);
  if (!bRes) {
    return outcome::failure(std::error_code(::GetLastError(), std::system_category()));
  }

  BITMAP bmpScreen;
  // Get the BITMAP from the HBITMAP.
  ::GetObject(hbm, sizeof(BITMAP), &bmpScreen);

  BITMAPFILEHEADER   bmfHeader;
  BITMAPINFOHEADER   bi;

  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = bmpScreen.bmWidth;
  bi.biHeight = bmpScreen.bmHeight;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  ::GetDIBits(hdc, hbm, 0, bmpScreen.bmHeight, nullptr, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

  DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
  DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
  
  std::vector<char> data = std::vector<char>(dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
  char* ptr = data.data();

  int res = ::GetDIBits(hdc, hbm, 0, bmpScreen.bmHeight,
			ptr + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
  if (res == 0) {
    return outcome::failure(std::error_code(res, std::system_category()));
  }

  bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
  bmfHeader.bfSize = dwSizeofDIB;
  bmfHeader.bfType = 0x4D42;

  std::memcpy(ptr, &bmfHeader, sizeof(BITMAPFILEHEADER));
  std::memcpy(ptr + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));

  return outcome::success(data);
}

outcome::result<void> write_dc_to_file(HDC hdc, DWORD default_size, std::filesystem::path p) {
  std::ofstream ofs(p, std::ios::binary | std::ios::trunc);

  std::vector<char> bitmap;
  BOOST_OUTCOME_TRY(bitmap, write_dc_to_bitmap(hdc, default_size));

  ofs.write(bitmap.data(), bitmap.size());
  return outcome::success();
}
  
void send_message(common::shared_memory* ptr, common::message_t msg) {
  using namespace boost::interprocess;
  
  scoped_lock<interprocess_mutex> lk(ptr->m_mut);
  ptr->m_buf.push_back(msg);
  ptr->m_cv.notify_one();
}

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
