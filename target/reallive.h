#pragma once

#include "base.h"
#include "hook_manager.h"

#include <memory>
#include <string>

namespace targets {
	class reallive
	{
		static constexpr std::wstring_view s_target_name = L"reallive";

		using GetGlyphOutlineA_t = decltype(&GetGlyphOutlineA);
		using d3d_draw_t = uint32_t(__stdcall*)(uint32_t);

		GetGlyphOutlineA_t m_pfGlyphOutlineAOriginal = nullptr;
		d3d_draw_t m_pfD3DDrawOriginal = nullptr;

		static DWORD WINAPI fakeGetGlyphOutlineA(HDC hdc, UINT uChar, UINT fuFormat, LPGLYPHMETRICS lpgm, DWORD cjBuffer, LPVOID pvBuffer, const MAT2* lpmat2);
		static uint32_t __stdcall fake_d3d_draw(uint32_t arg);
	public:

		static std::unique_ptr<reallive> create(hook_manager& hm, HANDLE hPipe);
		void close(hook_manager& hm);
	};
}

