#include "ever17.h"

using namespace targets::kid;

std::shared_ptr<ever17> targets::kid::ever17::try_create(hook_manager& hm, wil::shared_hfile pipe)
{
	std::filesystem::path module_path = common::get_module_file_name_w();

	auto filename = std::move(module_path).filename().wstring();
	std::transform(filename.begin(), filename.end(), filename.begin(), towupper);

	if (filename == L"EVER17.EXE")
	{
		return nullptr;
	}

	auto ptr = std::shared_ptr<ever17>();

	ptr->m_pipe = pipe;

	return ptr;
}
