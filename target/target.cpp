#include "target.h"

#include <cstdint>
#include <format>
#include <string>
#include <vector>
#include <functional>

using namespace std;

hook_manager hm;

void print_message(wstring const& msg) {
    MessageBoxW(0, msg.c_str(), L"", MB_ICONINFORMATION);
}

//void print_glyph_format(uint32_t uFormat) {
//    switch (uFormat) {
//    case GGO_BEZIER:
//        print_message(L"bezier");
//        break;
//    case GGO_BITMAP:
//        print_message(L"bitmap");
//        break;
//    case GGO_GLYPH_INDEX:
//        print_message(L"bezier");
//        break;
//    case GGO_GRAY2_BITMAP:
//        print_message(L"gray2");
//        break;
//    case GGO_GRAY4_BITMAP:
//        print_message(L"gray2");
//        break;
//    case GGO_GRAY8_BITMAP:
//        print_message(L"gray2");
//        break;
//    case GGO_METRICS:
//        print_message(L"metrics");
//        break;
//    case GGO_NATIVE:
//        print_message(L"native");
//        break;
//    default:
//        print_message(L"unknown");
//        break;
//    }
//}

HANDLE hPipe = NULL;

void create_pipe() {
    hPipe = CreateFileW(
        L"\\\\.\\pipe\\hookdict_pipe",
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
}

std::unique_ptr<targets::target_interface<hook_manager>> g_target = nullptr;
glyph_cache gc;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        print_message(L"Attached");
        create_pipe();

        if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) {
            print_message(L"Failed to create pipe!");
            return true;
        }

        DWORD dwMode = PIPE_READMODE_MESSAGE;
        bool bRes = SetNamedPipeHandleState(
            hPipe,
            &dwMode,
            nullptr,
            nullptr
            );

        if (!bRes) {
            print_message(L"Failed to set pipe handle state!");
            return true;
        }

        pipe_write_string(hPipe, L"Target DLL: Connected to pipe");

        if (!hm.init()) {
            pipe_write_string(hPipe, L"init error");
        }

        g_target = targets::reallive::create(hm, hPipe);
        if (g_target == nullptr) {
            pipe_write_string(hPipe, L"Failed to create target object!");
        }
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        if (hPipe != NULL) {
            pipe_write_string(hPipe, L"detached");
            return true;
        }
        
        g_target->close(hm);
        g_target = nullptr;
        
        hm.uninit();
    }
    return TRUE;
}