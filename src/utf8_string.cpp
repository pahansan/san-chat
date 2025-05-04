#include "utf8_string.hpp"

#include <codecvt>
#include <locale>

size_t utf8_strlen(const std::string& utf8_string)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide_string = converter.from_bytes(utf8_string);
    return wide_string.size();
}

void utf8_pop_back(std::string& s)
{
    if (s.empty())
        return;

    size_t pos = s.size() - 1;
    size_t erase_len = 1;

    while (pos > 0 && (s[pos] & 0xC0) == 0x80) {
        --pos;
        ++erase_len;
    }

    if (pos < s.size()) {
        const unsigned char first = s[pos];
        if ((first & 0xF8) == 0xF0)
            erase_len = 4;
        else if ((first & 0xF0) == 0xE0)
            erase_len = 3;
        else if ((first & 0xE0) == 0xC0)
            erase_len = 2;
    }

    if (erase_len > s.size() - pos) {
        erase_len = s.size() - pos;
    }

    s.resize(s.size() - erase_len);
}