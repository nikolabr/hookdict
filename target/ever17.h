#pragma once

#include "base.h"

namespace targets {
	namespace kid {
		class ever17
		{
			wil::shared_hfile m_pipe;

		public:
			static constexpr std::wstring_view s_target_name = L"kid::ever17";

			static std::shared_ptr<ever17> create(hook_manager& hm, wil::shared_hfile pipe);
		};
	}
}
