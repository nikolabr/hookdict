#pragma once

#include "common.h"

#include "fastcorr.h"
#include "hook_manager.h"

#include <windows.h>

#include <string>
#include <vector>
#include <list>
#include <format>

struct glyph_info {
	unsigned int m_val = 0;
	std::vector<unsigned char> m_buffer;

	unsigned int m_x = 0;
	unsigned int m_y = 0;

	glyph_info(unsigned int char_val, unsigned char* ptr, std::size_t n, unsigned int x, unsigned int y);
};

struct glyph_cache {
	std::list<glyph_info> m_glyphs;
};

static void write_to_pipe(wil::shared_hfile pipe_handle, auto&& msg)
{
	std::wstring pipemsg(msg);
	BOOL res = ::WriteFile(pipe_handle.get(), pipemsg.c_str(), pipemsg.size(), nullptr, nullptr);
	THROW_IF_WIN32_BOOL_FALSE(res);
}

namespace targets {
	template <typename T>
	void target_log(const T& target, std::wstring const& wstr) {
		std::wstring msg = std::format(L"target {}: {}", target.s_target_name, wstr);
		pipe_write_string(target.m_hPipe, msg);
	}
}
