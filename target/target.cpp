
#include "target.h"
#include "base.h"
#include "common.h"
#include "ever17.h"
#include "hook_manager.h"

#include <any>
#include <exception>
#include <utility>

#include <rpc/client.h>

static hook_manager g_hm;
static std::any g_target;
static std::unique_ptr<rpc::client> g_client;

template <typename T>
using target_factory_t = std::shared_ptr<T> (*)(hook_manager&, rpc::client&);

template <typename T>
static std::shared_ptr<T>
try_to_create_target(target_factory_t<T> target_factory) {
  return target_factory(g_hm, *g_client);
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


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    write_msg(L"Target connected");
    
    std::filesystem::path module_path = common::get_module_file_name_w();
    
    g_hm.init();
    
    g_client = std::make_unique<rpc::client>("127.0.0.1", common::hookdict_port);
    
    auto target_ptr = try_to_create_target(targets::kid::ever17::try_create);
    
    if (!target_ptr) {
      write_msg(L"Failed to discover target");
    } else {
      g_target = std::any(std::move(target_ptr));
    }

  } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    g_target.reset();
    g_hm.uninit();
  }
  return TRUE;
}
