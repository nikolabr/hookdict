#pragma once

#include "base.h"
#include "common.h"
#include "hook_manager.h"

#include <type_traits>

namespace targets {
  namespace kid {
    class ever17
    {
      wil::shared_hfile m_pipe;

      std::string m_prev;
    public:
      struct textoutw_hook : hooks::hook_base<decltype(&TextOutA), HDC, int, int, LPCSTR, int> {
	using hook_base::hook_base;
	
	static __attribute__((stdcall)) return_t fake_call(HDC__ *hdc, int x, int y, const char *lpString, int c);
      };
      
      static constexpr wchar_t s_target_name[] = L"kid::ever17";

      static std::shared_ptr<ever17> try_create(hook_manager& hm, wil::shared_hfile pipe);

      ~ever17();
    private:
      textoutw_hook m_textoutw_hook{TextOutA};
    };
  }
}
