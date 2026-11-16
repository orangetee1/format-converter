#pragma once

// Общая база читателей JSON, TOML и XML: курсор, ошибки, проверка входа,
// а также общие для JSON и TOML строки и числа.

#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "core/cursor.hpp"
#include "core/error.hpp"
#include "core/text.hpp"
#include "core/value.hpp"

namespace lab4 {

inline constexpr auto to_value = [](auto&& x) {
    return Value(std::forward<decltype(x)>(x));
};

class ParserBase {
protected:
    explicit ParserBase(std::string_view text) noexcept : cursor_(text) {}

    template <class Self>
    [[nodiscard]] std::unexpected<Error> fail(
        this const Self& self, std::string message,
        std::optional<Position> at = std::nullopt) {
        const Position p = at.value_or(self.cursor_.position());
        return std::unexpected(Error{.format = std::string(Self::kFormatName),
                                     .line = p.line,
                                     .column = p.column,
                                     .message = std::move(message)});
    }

    template <class Self>
    [[nodiscard]] Result<void> expect(this Self& self, char expected) {
        self.skip_ws();
        if (self.cursor_.consume(expected)) {
            return {};
        }
        return self.fail(std::format("expected '{}', found {}", expected,
                                     self.describe_next()));
    }

    [[nodiscard]] std::string describe_next() const {
        if (cursor_.eof()) {
            return "end of input";
        }
        const char c = cursor_.peek();
        return c == '\n' || c == '\r' ? "end of line" : std::format("'{}'", c);
    }

    template <class Self>
    [[nodiscard]] Result<void> require_text(this const Self& self) {
        for (Cursor probe = self.cursor_; !probe.eof(); probe.advance()) {
            if (!is_allowed_char(probe.peek())) {
                return self.fail(
                    std::format("byte 0x{:02X} is not allowed: only printable "
                                "ASCII, tabs and newlines",
                                static_cast<unsigned char>(probe.peek())),
                    probe.position());
            }
        }
        return {};
    }

    template <class Self>
    [[nodiscard]] Result<std::string> parse_basic_string(this Self& self,
                                                         bool allow_tab) {
        Cursor& cursor = self.cursor_;
        const Position start = cursor.position();
        cursor.advance();  // '"'
        std::string out;
        while (true) {
            const Position at = cursor.position();
            const char c = cursor.peek();
            if (cursor.eof() || c == '\n' || c == '\r') {
                return self.fail("unterminated string", start);
            }
            cursor.advance();
            if (c == '"') {
                return out;
            }
            if (c == '\t' && !allow_tab) {
                return self.fail("tab in a string must be escaped", at);
            }
            if (c != '\\') {
                out += c;
                continue;
            }
            switch (const char escape = cursor.advance()) {
                case '"':
                case '\\':
                case '/':
                    out += escape;
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 't':
                    out += '\t';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case 'u': {
                    int code = 0;
                    for (int i = 0; i < 4; ++i) {
                        const int digit = hex_value(cursor.peek());
                        if (digit < 0) {
                            return self.fail("expected 4 hex digits after \\u");
                        }
                        code = code * 16 + digit;
                        cursor.advance();
                    }
                    if (code >= 0x80 ||
                        !is_allowed_char(static_cast<char>(code))) {
                        return self.fail(
                            std::format("\\u{:04X} is not allowed: only "
                                        "printable ASCII, tabs and newlines",
                                        code),
                            at);
                    }
                    out += static_cast<char>(code);
                    break;
                }
                default:
                    return self.fail("invalid escape sequence", at);
            }
        }
    }

    // Число по грамматике раздела 3: -?(0|[1-9]\d*)(\.\d+)?([eE][+-]?\d+)?
    // TOML разрешает ещё ведущий '+' (allow_plus). С '.' или 'e' — float,
    // иначе int; выход за пределы int64 или double — ошибка.
    template <class Self>
    [[nodiscard]] Result<Value> parse_number_value(this Self& self,
                                                   bool allow_plus) {
        Cursor& cursor = self.cursor_;
        const Position start = cursor.position();
        const std::size_t from = cursor.offset();
        // Съесть цифры, вернуть их количество.
        const auto digits = [&cursor] {
            const std::size_t before = cursor.offset();
            while (is_digit(cursor.peek())) {
                cursor.advance();
            }
            return cursor.offset() - before;
        };

        if (!cursor.consume('-') && allow_plus) {
            cursor.consume('+');
        }
        if (cursor.consume('0')) {
            if (is_digit(cursor.peek())) {
                return self.fail("leading zeros are not allowed", start);
            }
        } else if (digits() == 0) {
            return self.fail(std::format("expected a digit, found {}",
                                         self.describe_next()));
        }
        bool is_float = false;
        if (cursor.consume('.')) {
            is_float = true;
            if (digits() == 0) {
                return self.fail("expected a digit after '.'");
            }
        }
        if (cursor.consume('e') || cursor.consume('E')) {
            is_float = true;
            if (!cursor.consume('+')) {
                cursor.consume('-');
            }
            if (digits() == 0) {
                return self.fail("expected a digit in the exponent");
            }
        }

        std::string_view text = cursor.slice_from(from);
        if (text.starts_with('+')) {
            text.remove_prefix(1);  // from_chars не принимает '+'
        }
        if (is_float) {
            if (const auto number = parse_number<double>(text)) {
                return Value(*number);
            }
            return self.fail(std::format("{} is out of range of double", text),
                             start);
        }
        if (const auto number = parse_number<std::int64_t>(text)) {
            return Value(*number);
        }
        return self.fail(std::format("{} does not fit into int64", text),
                         start);
    }

    Cursor cursor_;
};

}  // namespace lab4
