#include "base.h"

#include "msg.h"

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

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
