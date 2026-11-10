#pragma once

#include <cstddef>
#include <string_view>

#include "core/error.hpp"

namespace lab4 {

class Cursor final {
public:
    explicit constexpr Cursor(std::string_view text) noexcept : text_(text) {}

    [[nodiscard]] constexpr bool eof() const noexcept {
        return offset_ >= text_.size();
    }

    [[nodiscard]] constexpr char peek(std::size_t ahead = 0) const noexcept {
        const std::size_t index = offset_ + ahead;
        return index < text_.size() ? text_[index] : '\0';
    }

    char advance() noexcept;

    bool consume(char expected) noexcept;

    bool consume(std::string_view expected) noexcept;

    [[nodiscard]] constexpr bool starts_with(
        std::string_view prefix) const noexcept {
        return rest().starts_with(prefix);
    }

    [[nodiscard]] constexpr Position position() const noexcept {
        return position_;
    }
    [[nodiscard]] constexpr std::size_t offset() const noexcept {
        return offset_;
    }

    [[nodiscard]] constexpr std::string_view rest() const noexcept {
        return text_.substr(offset_ < text_.size() ? offset_ : text_.size());
    }

    [[nodiscard]] constexpr std::string_view slice_from(
        std::size_t from) const noexcept {
        return text_.substr(from, offset_ - from);
    }

private:
    std::string_view text_;
    std::size_t offset_ = 0;
    Position position_{};
};

}  // namespace lab4
