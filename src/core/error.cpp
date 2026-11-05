#include "core/error.hpp"

#include <ostream>
#include <string>

namespace lab4 {

namespace {

std::optional<std::string_view> nth_line(std::string_view source,
                                         std::size_t line_number) {
    std::size_t begin = 0;
    for (std::size_t current = 1; current < line_number; ++current) {
        const std::size_t newline = source.find('\n', begin);
        if (newline == std::string_view::npos) {
            return std::nullopt;  // строк меньше, чем line_number
        }
        begin = newline + 1;
    }

    std::string_view line = source.substr(begin);
    line = line.substr(0, line.find('\n'));
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    return line;
}

std::string caret_padding(std::string_view line, std::size_t column) {
    std::string padding;
    std::size_t current_column = 1;
    for (std::size_t i = 0; i < line.size() && current_column < column; ++i) {
        const auto byte = static_cast<unsigned char>(line[i]);
        if ((byte & 0xC0U) == 0x80U) {
            continue;  // байт продолжения: тот же символ, позиция не растёт
        }
        padding += (byte == '\t') ? '\t' : ' ';
        ++current_column;
    }

    if (current_column < column) {
        padding.append(column - current_column, ' ');
    }
    return padding;
}

}  // namespace

std::string render_error(const Error& error, std::string_view source,
                         std::string_view source_name) {
    // Строка 1: "error[<kind>]: [<format>: ]<message>"
    std::string out = "error[";
    out += to_string(error.kind);
    out += "]: ";
    if (!error.format.empty()) {
        out += error.format;
        out += ": ";
    }
    out += error.message;
    out += '\n';

    if (!error.position) {
        return out;
    }
    const Position position = *error.position;

    // Строка 2: " --> <source_name>:<line>:<column>"
    out += " --> ";
    out += source_name;
    out += ':';
    out += std::to_string(position.line);
    out += ':';
    out += std::to_string(position.column);
    out += '\n';

    const std::optional<std::string_view> line =
        nth_line(source, position.line);
    if (!line) {
        return out;
    }

    // Строки 3–4: фрагмент исходника и «каретка».
    const std::string number = std::to_string(position.line);
    const std::string gutter(number.size(), ' ');

    out += "  ";
    out += number;
    out += " | ";
    out += *line;
    out += '\n';

    out += "  ";
    out += gutter;
    out += " | ";
    out += caret_padding(*line, position.column);
    out += "^\n";
    return out;
}

std::ostream& operator<<(std::ostream& os, const Position& position) {
    return os << position.line << ':' << position.column;
}

std::ostream& operator<<(std::ostream& os, ErrorKind kind) {
    return os << to_string(kind);
}

std::ostream& operator<<(std::ostream& os, const Error& error) {
    os << '[' << error.kind << "] ";
    if (!error.format.empty()) {
        os << error.format << ": ";
    }
    os << error.message;
    if (error.position) {
        os << " at " << *error.position;
    }
    return os;
}

}  // namespace lab4
