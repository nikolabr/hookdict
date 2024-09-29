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
#include <gdiplus.h>

using namespace targets::kid;
using namespace Gdiplus;

static std::shared_ptr<ever17> g_ever17 = nullptr;
static ULONG_PTR gdiplusToken;

HRESULT GetEncoderClsid(const std::wstring& format, GUID* pGuid)
{
	HRESULT hr = S_OK;
	UINT  nEncoders = 0;          // number of image encoders
	UINT  nSize = 0;              // size of the image encoder array in bytes
	std::vector<BYTE> spData;
	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
	Gdiplus::Status status;
	bool found = false;

	if (format.empty() || !pGuid)
	{
		hr = E_INVALIDARG;
	}

	if (SUCCEEDED(hr))
	{
		*pGuid = GUID_NULL;
		status = Gdiplus::GetImageEncodersSize(&nEncoders, &nSize);

		if ((status != Gdiplus::Ok) || (nSize == 0))
		{
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{

		spData.resize(nSize);
		pImageCodecInfo = (Gdiplus::ImageCodecInfo*)&spData.front();
		status = Gdiplus::GetImageEncoders(nEncoders, nSize, pImageCodecInfo);

		if (status != Gdiplus::Ok)
		{
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
	{
		for (UINT j = 0; j < nEncoders && !found; j++)
		{
			if (pImageCodecInfo[j].MimeType == format)
			{
				*pGuid = pImageCodecInfo[j].Clsid;
				found = true;
			}
		}

		hr = found ? S_OK : E_FAIL;
	}

	return hr;
}

common::shared_memory* targets::kid::ever17::get_shm_ptr() {
	void* p = m_region.get_address();
	return static_cast<common::shared_memory*>(p);
}

std::shared_ptr<ever17>
targets::kid::ever17::try_create(hook_manager& hm,
	boost::interprocess::mapped_region&& region) {
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

	auto* shm_ptr = ptr->get_shm_ptr();
	send_message(shm_ptr,
		common::target_connected_message_t{ .m_window_name{
			"Ever17 - the out of infinity -  Premium Edition DVD"} });

	ptr->m_main_wnd = ::FindWindow(
		nullptr, "Ever17 - the out of infinity -  Premium Edition DVD");
	if (ptr->m_main_wnd == NULL) {
		throw std::runtime_error("Could not find main window!");
	}

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	return ptr;
}

ever17::textoutw_hook::return_t WINAPI ever17::textoutw_hook::fake_call(
	HDC hdc, int x, int y, LPCSTR lpString, int c) {
	std::string out(lpString);
	uint32_t codepage = GetACP();

	auto* shm_ptr = g_ever17->get_shm_ptr();
	send_message(shm_ptr, common::target_generic_message_t{ .m_text{lpString},
														   .m_cp{codepage} });

	int res = g_ever17->m_textoutw_hook.m_old_ptr(hdc, x, y, lpString, c);

	if (g_ever17->m_main_wnd != NULL) {
		/*
		RECT rect;
		::GetClientRect(g_ever17->m_main_wnd, &rect);

		HDC hdc = ::GetDC(g_ever17->m_main_wnd);

		DWORD width = rect.right - rect.left;
		DWORD height = rect.bottom - rect.top;

		Bitmap* bmp = new Bitmap(::CreateCompatibleBitmap(hdc, width, height), 0);
		Graphics bmpGraphics(bmp);

		HDC hBmpDC = bmpGraphics.GetHDC();
		DWORD lines = ::BitBlt(hBmpDC, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

		CLSID clsid;
		GetEncoderClsid(L"image/bmp", &clsid);
		bmp->Save(L"image.bmp", &clsid, nullptr);

		send_message(shm_ptr, common::bitmap_update_message_t{
			.m_width{(long)width},
			.m_height{(long)height},
			.m_lines{lines}
			});

		::ReleaseDC(NULL, hdc);
		*/
	}

	return res;
}

ever17::bitblt_hook::return_t ever17::bitblt_hook::fake_call(HDC   hdc,
	int   x,
	int   y,
	int   cx,
	int   cy,
	HDC   hdcSrc,
	int   x1,
	int   y1,
	DWORD rop) {


}

ever17::~ever17() {
	auto* shm_ptr = get_shm_ptr();
	send_message(shm_ptr, common::target_close_message_t{});

	GdiplusShutdown(gdiplusToken);

	g_ever17 = nullptr;
}
