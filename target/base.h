#pragma once

#include "common.h"
#include "hooks.h"
#include "msg.h"

void send_message(common::shared_memory *ptr, common::message_t msg);

static void write_msg(wchar_t const *msg) {
  ::MessageBoxW(nullptr, msg, L"Target message", MB_OK);
}
