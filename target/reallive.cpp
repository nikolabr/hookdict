#include "reallive.h"

#include <functional>
#include <format>
#include <sstream>
#include <iostream>

// Points to the current instance of the target, if it exists
constinit targets::reallive* g_reallive_ptr = nullptr;

namespace targets {
    /*
    DWORD WINAPI reallive::fakeGetGlyphOutlineA(HDC hdc, UINT uChar, UINT fuFormat, LPGLYPHMETRICS lpgm, DWORD cjBuffer, LPVOID pvBuffer, const MAT2* lpmat2) {
        DWORD dwRet = g_reallive_ptr->m_pfGlyphOutlineAOriginal(hdc, uChar, fuFormat, lpgm, cjBuffer, pvBuffer, lpmat2);

        std::wostringstream woss;

        woss << std::hex << uChar << std::dec << " X: " << lpgm->gmBlackBoxX << " Y: " << lpgm->gmBlackBoxY
            << " size: " << cjBuffer << " format: " << fuFormat;
        target_log(*g_reallive_ptr, L"GetGlyphOutlineACall: " + woss.str());

        if (cjBuffer > 0 && pvBuffer != nullptr) {
            unsigned char* ptr = reinterpret_cast<unsigned char*>(pvBuffer);
            glyph_info gi(uChar, ptr, cjBuffer, 
                lpgm->gmBlackBoxX, lpgm->gmBlackBoxY);
            g_reallive_ptr->gc.m_glyphs.emplace_back(std::move(gi));
        }

        return dwRet;
    }

    uint32_t __stdcall reallive::fake_d3d_draw(uint32_t arg) {
        uint32_t ret = g_reallive_ptr->m_pfD3DDrawOriginal(arg);
        // target_log(*g_reallive_ptr, L"d3d_dll_draw");
        return ret;
    }

    std::unique_ptr<reallive> reallive::create(hook_manager& hm, HANDLE hPipe)
    {
        std::unique_ptr reallive_ptr = std::make_unique<reallive>(); 
        pipe_write_string(hPipe, L"Create target reallive");

        reallive_ptr->m_hPipe = hPipe;

        reallive_ptr->m_pfGlyphOutlineAOriginal = hm.enable_hook(GetGlyphOutlineA, &fakeGetGlyphOutlineA);
        if (reallive_ptr->m_pfGlyphOutlineAOriginal == nullptr) {
            pipe_write_string(hPipe, L"Failed to set GetGlyphOutlineA hook");
            return nullptr;
        }

        HINSTANCE hD3DModule = GetModuleHandleW(L"RL_D3D.DLL");
        if (hD3DModule == NULL) {
            pipe_write_string(hPipe, L"Could not load RL_D3D.DLL");
            return nullptr;
        }

        void* lpD3DDraw = GetProcAddress(hD3DModule, "d3d_dll_draw");
        if (hD3DModule == nullptr) {
            pipe_write_string(hPipe, L"Could not find d3d_dll_draw in RL_D3D.DLL");
            return nullptr;
        }

        g_reallive_ptr->m_pfD3DDrawOriginal = hm.enable_hook(reinterpret_cast<d3d_draw_t>(lpD3DDraw), &fake_d3d_draw);
        if (g_reallive_ptr->m_pfD3DDrawOriginal == nullptr) {
            pipe_write_string(hPipe, L"Failed to set d3d_dll_draw hook");
            return nullptr;
        }

        g_reallive_ptr = reallive_ptr.get();
        pipe_write_string(hPipe, L"Successfully created target reallive");

        return reallive_ptr;
    }

    void reallive::close(hook_manager& hm)
    {
        hm.disable_hook(GetGlyphOutlineA);
        g_reallive_ptr = nullptr;
    }
    */
}