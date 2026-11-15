#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>

namespace lab4 {

[[nodiscard]] constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

[[nodiscard]] constexpr int hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') {
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

bool encode_utf8(char32_t code_point, std::string& out);

template <std::integral T>
[[nodiscard]] std::expected<T, std::errc> parse_number(std::string_view text,
                                                       int base = 10) {
    T value{};
    const char* const end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), end, value, base);
    if (ec != std::errc{}) {
        return std::unexpected(ec);
    }
    if (ptr != end) {
        return std::unexpected(std::errc::invalid_argument);
    }
    return value;
}

template <std::floating_point T>
[[nodiscard]] std::expected<T, std::errc> parse_number(std::string_view text) {
    T value{};
    const char* const end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), end, value);
    if (ec != std::errc{}) {
        return std::unexpected(ec);
    }
    if (ptr != end) {
        return std::unexpected(std::errc::invalid_argument);
    }
    return value;
}

template <class T>
    requires std::integral<T> || std::floating_point<T>
[[nodiscard]] std::string format_number(T value) {
    std::array<char, 64> buffer{};
    const auto [ptr, ec] =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);

    static_cast<void>(ec);

    std::string text(buffer.data(), ptr);
    if constexpr (std::floating_point<T>) {
        if (text.find_first_of(".en") == std::string::npos) {
            text += ".0";
        }
    }
    return text;
}

}  // namespace lab4
