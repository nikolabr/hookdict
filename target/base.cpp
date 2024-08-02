#include "base.h"
#include "common.h"

#include <minwindef.h>
#include <libloaderapi.h>

static wil::shared_hfile g_pipe;

glyph_info::glyph_info(unsigned int char_val, unsigned char* ptr, std::size_t n, unsigned int x, unsigned int y)
{
    m_val = char_val;
    m_buffer = std::vector<unsigned char>(n);
    std::copy(ptr, ptr + n, std::back_inserter(m_buffer));
    m_x = x;
    m_y = y;
}
