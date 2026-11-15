#pragma once

#include <cstddef>
#include <expected>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace lab4 {

struct Position {
    std::size_t line = 1;
    std::size_t column = 1;

    friend constexpr bool operator==(const Position&,
                                     const Position&) = default;
};

enum class ErrorKind {
    Syntax,       // нарушена грамматика формата: "expected ',' or '}'"
    Unsupported,  // корректно для формата, но вне общего подмножества: null
    Semantic,     // синтаксис верен, смысл нет: дубликат ключа
    Io,          // ошибка ввода-вывода: не удалось прочитать stdin
};

[[nodiscard]] constexpr std::string_view to_string(ErrorKind kind) noexcept {
    switch (kind) {
        case ErrorKind::Syntax:
            return "syntax";
        case ErrorKind::Unsupported:
            return "unsupported";
        case ErrorKind::Semantic:
            return "semantic";
        case ErrorKind::Io:
            return "io";
    }
    std::unreachable();
}

struct Error {
    ErrorKind kind = ErrorKind::Syntax;
    std::string format{};  // "json" / "toml" / "xml";
    std::string message{};
    std::optional<Position> position{};

    friend bool operator==(const Error&, const Error&) = default;
};

template <class T>
using Result = std::expected<T, Error>;

[[nodiscard]] std::string render_error(
    const Error& error, std::string_view source,
    std::string_view source_name = "<stdin>");

std::ostream& operator<<(std::ostream& os, const Position& position);
std::ostream& operator<<(std::ostream& os, ErrorKind kind);
std::ostream& operator<<(std::ostream& os, const Error& error);

}  // namespace lab4
