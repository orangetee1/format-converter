#pragma once

#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "core/cursor.hpp"
#include "core/error.hpp"

namespace lab4 {

class ParserBase {
protected:
    explicit ParserBase(std::string_view text) noexcept : cursor_(text) {}

    template <class Self>
    [[nodiscard]] std::unexpected<Error> fail(
        this const Self& self, ErrorKind kind, std::string message,
        std::optional<Position> at = std::nullopt) {
        return std::unexpected(Error{
            .kind = kind,
            .format = std::string(Self::kFormatName),
            .message = std::move(message),
            .position = at.value_or(self.cursor_.position()),
        });
    }

    template <class Self>
    [[nodiscard]] Result<void> expect(this Self& self, char expected) {
        self.skip_ws();
        if (self.cursor_.consume(expected)) {
            return {};
        }
        return self.fail(ErrorKind::Syntax,
                         std::format("expected '{}', found {}", expected,
                                     self.describe_next()));
    }

    [[nodiscard]] std::string describe_next() const {
        if (cursor_.eof()) {
            return "end of input";
        }
        const char c = cursor_.peek();
        if (c == '\n' || c == '\r') {
            return "end of line";
        }
        const auto byte = static_cast<unsigned char>(c);
        if (byte >= 0x20 && byte < 0x7F) {
            return std::format("'{}'", c);
        }
        return std::format("byte 0x{:02X}", byte);
    }

    Cursor cursor_;
};

}  // namespace lab4
