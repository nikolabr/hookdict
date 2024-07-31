#include "ever17.h"

using namespace targets::kid;

std::shared_ptr<ever17> targets::kid::ever17::create(hook_manager& hm, wil::shared_hfile pipe)
{
	auto ptr = std::shared_ptr<ever17>();

	ptr->m_pipe = pipe;

	return ptr;
}
