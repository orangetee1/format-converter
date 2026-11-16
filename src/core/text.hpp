#pragma once

// Мелкие помощники для текста: классы символов, числа, строка в кавычках.

#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace lab4 {

[[nodiscard]] constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

[[nodiscard]] constexpr int hex_value(char c) noexcept {
    if (is_digit(c)) {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

[[nodiscard]] constexpr bool is_allowed_char(char c) noexcept {
    return (c >= 0x20 && c < 0x7F) || c == '\t' || c == '\n' || c == '\r';
}

template <class T>
[[nodiscard]] std::optional<T> parse_number(std::string_view text) {
    T value{};
    const char* const end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), end, value);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }
    return value;
}

template <class T>
[[nodiscard]] std::string format_number(T value) {
    std::array<char, 32> buffer{};
    const auto result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    std::string text(buffer.data(), result.ptr);
    if constexpr (std::is_floating_point_v<T>) {
        if (text.find_first_of(".en") == std::string::npos) {
            text += ".0";
        }
    }
    return text;
}

[[nodiscard]] inline std::string quote(std::string_view text) {
    std::string out = "\"";
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\t':
                out += "\\t";
                break;
            case '\r':
                out += "\\r";
                break;
            default:
                out += c;
        }
    }
    return out + '"';
}

}  // namespace lab4
