// common.h : Header file for your target.

#pragma once

/*
  WIL options

  https://github.com/microsoft/wil/pull/454
 */
#define _Frees_ptr_
#define _Frees_ptr_opt_
#define _Post_z_
#define _Pre_maybenull_
#define _Pre_opt_valid_
#define _Pre_valid_
#define _Ret_opt_bytecap_(size)
#define _Translates_last_error_to_HRESULT_
#define _Translates_NTSTATUS_to_HRESULT_(status)
#define _Translates_Win32_to_HRESULT_(err)
#define InterlockedDecrementNoFence InterlockedDecrement
#define InterlockedIncrementNoFence InterlockedIncrement
#define WIL_NO_SLIM_EVENT

#include <winsock2.h>
#include <windows.h>

#include <filesystem>
#include <array>
#include <memory>

#include <wil/stl.h>

#define _MEMORY_
#include <wil/resource.h>

#include <wil/result_macros.h>

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
