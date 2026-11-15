#include "core/text.hpp"

namespace lab4 {

bool encode_utf8(char32_t code_point, std::string& out) {
    const auto byte = [&out](char32_t bits) {
        out += static_cast<char>(static_cast<unsigned char>(bits));
    };

    if (code_point >= 0xD800 && code_point <= 0xDFFF) {
        return false;
    }
    if (code_point < 0x80) {
        byte(code_point);
    } else if (code_point < 0x800) {
        byte(0xC0 | (code_point >> 6));
        byte(0x80 | (code_point & 0x3F));
    } else if (code_point < 0x10000) {
        byte(0xE0 | (code_point >> 12));
        byte(0x80 | ((code_point >> 6) & 0x3F));
        byte(0x80 | (code_point & 0x3F));
    } else if (code_point <= 0x10FFFF) {
        byte(0xF0 | (code_point >> 18));
        byte(0x80 | ((code_point >> 12) & 0x3F));
        byte(0x80 | ((code_point >> 6) & 0x3F));
        byte(0x80 | (code_point & 0x3F));
    } else {
        return false;  // за пределами Unicode
    }
    return true;
}

}  // namespace lab4
