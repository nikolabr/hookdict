// common.h : Header file for your target.

#pragma once

#include <windows.h>

#include <wil/result_macros.h>

#include <filesystem>
#include <array>

namespace common {
	constexpr static wchar_t hookdict_pipe_name[] = L"\\\\.\\pipe\\hookdict_pipe";

    static std::filesystem::path get_module_file_name_w(HMODULE handle)
    {
        std::array<wchar_t, MAX_PATH> out{};

        DWORD res = ::GetModuleFileNameW(handle, out.data(), out.size());

        THROW_LAST_ERROR_IF(res == 0);

        return std::filesystem::path(out.begin(), out.begin() + res);
    }

    static std::filesystem::path get_module_file_name_w()
    {
        return get_module_file_name_w(NULL);
    }

    template <typename H>
    static std::filesystem::path get_module_file_name_w(H&& handle)
    {
        return get_module_file_name_w(handle.get());
    }
}
