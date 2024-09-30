#pragma once

#include "common.h"
#include "hooks.h"
#include "msg.h"

#include <gdiplus.h>

#include <boost/outcome.hpp>
#include <boost/outcome/outcome.hpp>

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

void send_message(common::shared_memory *ptr, common::message_t msg);

outcome::result<std::vector<char>> write_hbitmap_to_memory(HBITMAP hbm);
outcome::result<std::vector<char>> write_dc_to_memory(HDC hdc, DWORD default_size);
outcome::result<void> write_dc_to_file(HDC hdc, DWORD default_size, std::filesystem::path p);

static void write_msg(wchar_t const *msg) {
  ::MessageBoxW(nullptr, msg, L"Target message", MB_OK);
}

IStream* get_img_buf_stream(common::shared_memory* shm_ptr);
