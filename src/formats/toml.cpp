

#include <format>
#include <set>
#include <string>

#include "core/overloaded.hpp"
#include "core/parser_base.hpp"
#include "formats/formats.hpp"

namespace lab4 {

namespace {

constexpr bool is_key_char(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_digit(c) ||
           c == '_' || c == '-';
}

class TomlReader final : public ParserBase {
public:
    static constexpr std::string_view kFormatName = Toml::name;

    explicit TomlReader(std::string_view text) noexcept : ParserBase(text) {}

    void skip_ws() noexcept {
        while (cursor_.peek() == ' ' || cursor_.peek() == '\t') {
            cursor_.advance();
        }
    }

    Result<Value> parse_document() {
        if (auto text = require_text(); !text) {
            return std::unexpected(std::move(text.error()));
        }
        while (true) {
            skip_ws();
            if (cursor_.eof()) {
                return std::move(root_);
            }
            const char c = cursor_.peek();
            Result<void> line;
            if (c == '[') {
                line = parse_header();
            } else if (c != '#' && c != '\n' && c != '\r') {
                line = parse_key_value(*current_);
            }
            if (line) {
                line = end_of_line();
            }
            if (!line) {
                return std::unexpected(std::move(line.error()));
            }
        }
    }

private:
    void skip_comment() noexcept {
        if (cursor_.peek() != '#') {
            return;
        }
        while (!cursor_.eof() && cursor_.peek() != '\n') {
            cursor_.advance();
        }
    }

    bool consume_newline() noexcept {
        return cursor_.consume('\n') || cursor_.consume("\r\n");
    }

    Result<void> end_of_line() {
        skip_ws();
        skip_comment();
        if (cursor_.eof() || consume_newline()) {
            return {};
        }
        return fail(
            std::format("expected end of line, found {}", describe_next()));
    }

    void skip_ws_and_newlines() noexcept {
        do {
            skip_ws();
            skip_comment();
        } while (consume_newline());
    }

    Result<std::string> parse_key() {
        skip_ws();
        const Position at = cursor_.position();
        const std::size_t from = cursor_.offset();
        while (is_key_char(cursor_.peek())) {
            cursor_.advance();
        }
        std::string key(cursor_.slice_from(from));
        if (key.empty()) {
            return fail(
                std::format("expected a key, found {}", describe_next()));
        }
        if (!is_valid_key(key)) {
            return fail(std::format("unsupported key \"{}\"", key), at);
        }
        return key;
    }

    Result<void> parse_header() {
        cursor_.advance();
        Object* table = &root_;
        std::string path;
        while (true) {
            skip_ws();
            const Position at = cursor_.position();
            auto key = parse_key();
            if (!key) {
                return std::unexpected(std::move(key.error()));
            }
            path += (path.empty() ? "" : ".") + *key;
            skip_ws();
            const bool last = cursor_.peek() == ']';

            const auto [it, inserted] = table->try_emplace(*key, Object{});
            if (inserted) {
                tables_.insert(path);
            } else if (!it->second.is<Object>() || !tables_.contains(path) ||
                       (last && defined_.contains(path))) {
                return fail(std::format("'{}' is already defined", path), at);
            }

            table = &it->second.get<Object>();
            if (last) {
                break;
            }
            if (!cursor_.consume('.')) {
                return fail(std::format("expected '.' or ']', found {}",
                                        describe_next()));
            }
        }
        cursor_.advance();
        defined_.insert(path);
        current_ = table;
        return {};
    }

    Result<void> parse_key_value(Object& table) {
        skip_ws();
        const Position at = cursor_.position();
        auto key = parse_key();
        if (!key) {
            return std::unexpected(std::move(key.error()));
        }
        if (table.contains(*key)) {
            return fail(std::format("duplicate key '{}'", *key), at);
        }
        if (auto eq = expect('='); !eq) {
            return eq;
        }
        skip_ws();
        auto value = parse_value();
        if (!value) {
            return std::unexpected(std::move(value.error()));
        }
        table.emplace(std::move(*key), std::move(*value));
        return {};
    }

    Result<Value> parse_value() {
        const char c = cursor_.peek();
        if (c == '"') {
            return parse_basic_string(true).transform(to_value);
        }
        if (c == '\'') {
            return parse_literal_string().transform(to_value);
        }
        if (c == '[') {
            return parse_array();
        }
        if (c == '{') {
            return parse_inline_table();
        }
        if (c == '+' || c == '-' || is_digit(c)) {
            return parse_number_value(true);
        }
        if (cursor_.consume("true")) {
            return Value(true);
        }
        if (cursor_.consume("false")) {
            return Value(false);
        }
        return fail(std::format("expected a value, found {}", describe_next()));
    }

    Result<std::string> parse_literal_string() {
        const Position start = cursor_.position();
        cursor_.advance();
        const std::size_t from = cursor_.offset();
        while (!cursor_.eof() && cursor_.peek() != '\'' &&
               cursor_.peek() != '\n' && cursor_.peek() != '\r') {
            cursor_.advance();
        }
        std::string text(cursor_.slice_from(from));
        if (!cursor_.consume('\'')) {
            return fail("unterminated literal string", start);
        }
        return text;
    }

    Result<Value> parse_array() {
        cursor_.advance();
        Array array;
        while (true) {
            skip_ws_and_newlines();
            if (cursor_.consume(']')) {
                return array;
            }
            auto value = parse_value();
            if (!value) {
                return value;
            }
            array.push_back(std::move(*value));
            skip_ws_and_newlines();
            if (cursor_.consume(']')) {
                return array;
            }
            if (!cursor_.consume(',')) {
                return fail(std::format("expected ',' or ']', found {}",
                                        describe_next()));
            }
        }
    }

    Result<Value> parse_inline_table() {
        cursor_.advance();
        Object table;
        skip_ws();
        if (cursor_.consume('}')) {
            return table;
        }
        while (true) {
            if (auto entry = parse_key_value(table); !entry) {
                return std::unexpected(std::move(entry.error()));
            }
            skip_ws();
            if (cursor_.consume('}')) {
                return table;
            }
            if (!cursor_.consume(',')) {
                return fail(std::format("expected ',' or '}}', found {}",
                                        describe_next()));
            }
        }
    }

    Object root_;
    Object* current_ = &root_;
    std::set<std::string> tables_;
    std::set<std::string> defined_;
};

class TomlWriter final {
public:
    std::string run(const Object& root) {
        write_table(root, "");
        return std::move(out_);
    }

private:
    void write_table(const Object& table, const std::string& path) {
        for (const auto& [key, value] : table) {
            if (!value.is<Object>()) {
                out_ += key + " = ";
                write_inline(value);
                out_ += '\n';
            }
        }
        for (const auto& [key, value] : table) {
            if (const Object* child = value.get_if<Object>()) {
                const std::string child_path =
                    path.empty() ? key : path + '.' + key;
                out_ += std::format("{}[{}]\n", out_.empty() ? "" : "\n",
                                    child_path);
                write_table(*child, child_path);
            }
        }
    }

    void write_inline(const Value& value) {
        value.visit(Overloaded{
            [&](std::monostate) {},
            [&](bool b) { out_ += b ? "true" : "false"; },
            [&](std::int64_t i) { out_ += format_number(i); },
            [&](double d) { out_ += format_number(d); },
            [&](const std::string& s) { out_ += quote(s); },
            [&](const Array& array) {
                out_ += '[';
                for (const char* sep = ""; const Value& item : array) {
                    out_ += sep;
                    write_inline(item);
                    sep = ", ";
                }
                out_ += ']';
            },
            [&](const Object& object) {
                out_ += '{';
                for (const char* sep = " "; const auto& [key, item] : object) {
                    out_ += sep + key + " = ";
                    write_inline(item);
                    sep = ", ";
                }
                out_ += object.empty() ? "}" : " }";
            },
        });
    }

    std::string out_;
};

}  // namespace

Result<Value> Toml::read(std::string_view text) {
    return TomlReader(text).parse_document();
}

Result<std::string> Toml::write(const Value& value) {
    if (!value.is<Object>()) {
        return conversion_error(name, std::format("root must be a table, "
                                                  "found {}",
                                                  value.type_name()));
    }
    if (value.contains_null()) {
        return conversion_error(name, "unsupported type: null");
    }
    return TomlWriter{}.run(value.get<Object>());
}

}  // namespace lab4
