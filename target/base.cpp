#include "base.h"

#include <iterator>
#include <format>

bool pipe_write_string(HANDLE hPipe, std::string const& str) {
    DWORD nBytesWritten = 0;
    return WriteFile(
        hPipe,
        str.c_str(),
        str.size() * sizeof(std::string::value_type),
        &nBytesWritten,
        nullptr
    );
}

bool pipe_write_string(HANDLE hPipe, std::wstring const& str) {
    DWORD nBytesWritten = 0;
    return WriteFile(
        hPipe,
        str.c_str(),
        str.size() * sizeof(std::wstring::value_type),
        &nBytesWritten,
        nullptr
    );
}

glyph_info::glyph_info(unsigned int char_val, unsigned char* ptr, std::size_t n, unsigned int x, unsigned int y)
{
    m_val = char_val;
    m_buffer = std::vector<unsigned char>(n);
    std::copy(ptr, ptr + n, std::back_inserter(m_buffer));
    m_x = x;
    m_y = y;
}
