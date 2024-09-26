#pragma once

#include "base.h"
#include "common.h"
#include "hook_manager.h"

#include <type_traits>

namespace targets {
  namespace kid {
    class ever17
    {
      std::string m_prev;
    public:
      struct textoutw_hook : hooks::hook_base<decltype(&TextOutA), HDC, int, int, LPCSTR, int> {
	using hook_base::hook_base;
	using return_t = hook_base::return_t;
	
	static return_t WINAPI fake_call(HDC hdc, int x, int y, const char *lpString, int c);
      };
      
      static constexpr wchar_t s_target_name[] = L"kid::ever17";

      static std::shared_ptr<ever17> try_create(hook_manager& hm);
      
      ~ever17();
    private:
      textoutw_hook m_textoutw_hook{TextOutA};
    };
  }
}
