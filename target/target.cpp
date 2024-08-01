#include "target.h"

#include <any>

static hook_manager g_hm;

wil::shared_hfile g_pipe;

void create_pipe()
{
	g_pipe = wil::shared_hfile(CreateFileW(
		common::hookdict_pipe_name,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	));
}

std::any g_target;
glyph_cache gc;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		MessageBoxW(0, L"Target connected...", L"", MB_OK);
		create_pipe();

		if (!g_pipe) {
			MessageBoxW(0, L"Failed to create pipe", L"", MB_OK);
		}

		DWORD dwMode = PIPE_READMODE_MESSAGE;
		auto res = SetNamedPipeHandleState(
			g_pipe.get(),
			&dwMode,
			nullptr,
			nullptr
		);

		if (!res) { return res; }

		auto target_ptr = targets::kid::ever17::try_create(g_hm, g_pipe);
		if (!target_ptr) 
		{
			MessageBoxW(0, L"Failed to discover target", nullptr, MB_OK);
			write_to_pipe(g_pipe, L"Failed to discover target");
		}
		else {
			g_target = std::any(std::move(target_ptr));
			write_to_pipe(g_pipe, L"Target created");
		}
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
		g_target.reset();
		g_pipe.reset();
		g_hm.uninit();
	}
	return TRUE;
}
