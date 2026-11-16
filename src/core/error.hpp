#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace lab4 {

struct Error {
    std::string format;      // "json" / "toml" / "xml"
    std::size_t line = 0;    // с 1; 0 — без позиции
    std::size_t column = 0;  // с 1, в байтах
    std::string message;
};

template <class T>
using Result = std::expected<T, Error>;

[[nodiscard]] inline std::unexpected<Error> conversion_error(
    std::string_view format, std::string message) {
    return std::unexpected(
        Error{std::string(format), 0, 0, std::move(message)});
}

[[nodiscard]] std::string render_error(const Error& error,
                                       std::string_view source);

}  // namespace lab4
