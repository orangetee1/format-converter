#include "core/error.hpp"

#include <format>

namespace lab4 {

std::string render_error(const Error& error, std::string_view source) {
    if (error.line == 0) {
        return std::format("error: {}: {}\n", error.format, error.message);
    }
    std::string out = std::format("error: {}:{}:{}: {}\n", error.format,
                                  error.line, error.column, error.message);

    // пропускаем line - 1 переводов строки.
    std::size_t begin = 0;
    for (std::size_t i = 1; i < error.line; ++i) {
        const std::size_t newline = source.find('\n', begin);
        if (newline == std::string_view::npos) {
            return out;  // такой строки нет
        }
        begin = newline + 1;
    }
    std::string_view line = source.substr(begin);
    line = line.substr(0, line.find('\n'));
    if (line.ends_with('\r')) {
        line.remove_suffix(1);  // CRLF
    }

    std::string padding;
    for (std::size_t i = 0; i + 1 < error.column; ++i) {
        padding += (i < line.size() && line[i] == '\t') ? '\t' : ' ';
    }
    const std::string number = std::to_string(error.line);
    out += std::format("  {} | {}\n  {} | {}^\n", number, line,
                       std::string(number.size(), ' '), padding);
    return out;
}

}  // namespace lab4
