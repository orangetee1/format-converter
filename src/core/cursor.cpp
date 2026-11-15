#include "core/cursor.hpp"

namespace lab4 {

char Cursor::advance() noexcept {
    if (eof()) {
        return '\0';
    }
    const char c = text_[offset_++];

    if (c == '\n') {
        ++position_.line;
        position_.column = 1;
    } else {
        ++position_.column;
    }
    return c;
}

bool Cursor::consume(char expected) noexcept {
    if (eof() || text_[offset_] != expected) {
        return false;
    }
    advance();
    return true;
}

bool Cursor::consume(std::string_view expected) noexcept {
    if (!starts_with(expected)) {
        return false;
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        advance();
    }
    return true;
}

}  // namespace lab4
