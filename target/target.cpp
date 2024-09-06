
#include "target.h"
#include "base.h"
#include "common.h"
#include "ever17.h"
#include "hook_manager.h"

#include <any>
#include <tuple>
#include <utility>

static hook_manager g_hm;

template <typename T>
using target_factory_t = std::shared_ptr<T> (*)(hook_manager &,
                                                HANDLE);

template <typename T>
static std::shared_ptr<T>
try_to_create_target(target_factory_t<T> target_factory) {
  return target_factory(g_hm, g_pipe);
}

template <typename T>
static auto try_to_create_target(target_factory_t<T> target_factory,
                                 auto &&...other_factories) {
  auto target_ptr = try_to_create_target(target_factory);
  if (target_ptr) {
    return target_ptr;
  } else {
#ifdef DEBUG
    std::wstring msg(L"Target is not: ");
    msg += T::s_target_name;
    write_msg(msg.c_str());
#endif

    return try_to_create_target(other_factories...);
  }
}

std::any g_target;
glyph_cache gc;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    write_msg(L"Target connected");
    create_pipe();

    if (!g_pipe) {
      write_msg(L"Failed to connect to pipe");
    }

    DWORD dwMode = PIPE_READMODE_MESSAGE;
    auto res = SetNamedPipeHandleState(g_pipe, &dwMode, nullptr, nullptr);

    if (!res) {
      return true;
    }
      
    std::filesystem::path module_path = common::get_module_file_name_w();
    // write_to_pipe(g_pipe, module_path.string().c_str());

    g_hm.init();
    auto target_ptr = try_to_create_target(targets::kid::ever17::try_create);
    
    if (!target_ptr) {
      write_msg(L"Failed to discover target");
    } else {
      g_target = std::any(std::move(target_ptr));
      write_to_pipe(g_pipe, "Target created\n");
    }

    {
      HWND hwnd = GetTopWindow(NULL);
      static_assert(sizeof(HWND) == sizeof(uint32_t));
      std::string msg = std::to_string(reinterpret_cast<uint32_t>(hwnd));
      msg = "WINDOWHANDLE" + msg;
      write_to_pipe(g_pipe, msg);
    }
  } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    g_target.reset();
    CloseHandle(g_pipe);
    g_hm.uninit();
  }
  return TRUE;
}
