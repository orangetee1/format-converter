

#include <format>
#include <string>

#include "core/overloaded.hpp"
#include "core/parser_base.hpp"
#include "formats/formats.hpp"

namespace lab4 {

namespace {

class JsonReader final : public ParserBase {
public:
    static constexpr std::string_view kFormatName = Json::name;

    explicit JsonReader(std::string_view text) noexcept : ParserBase(text) {}

    void skip_ws() noexcept {
        for (char c = cursor_.peek();
             c == ' ' || c == '\t' || c == '\n' || c == '\r';
             c = cursor_.peek()) {
            cursor_.advance();
        }
    }

    Result<Value> parse_document() {
        auto value = require_text().and_then([this] { return parse_value(); });
        skip_ws();
        if (value && !cursor_.eof()) {
            return fail(std::format("unexpected {} after the root value",
                                    describe_next()));
        }
        return value;
    }

private:
    Result<Value> parse_value() {
        skip_ws();
        const char c = cursor_.peek();
        if (c == '{') {
            return parse_object();
        }
        if (c == '[') {
            return parse_array();
        }
        if (c == '"') {
            return parse_basic_string(false).transform(to_value);
        }
        if (c == '-' || is_digit(c)) {
            return parse_number_value(false);
        }
        if (cursor_.consume("true")) {
            return Value(true);
        }
        if (cursor_.consume("false")) {
            return Value(false);
        }
        if (cursor_.consume("null")) {
            return Value();
        }
        return fail(std::format("expected a value, found {}", describe_next()));
    }

    Result<Value> parse_object() {
        cursor_.advance();
        Object object;
        skip_ws();
        if (cursor_.consume('}')) {
            return object;
        }
        while (true) {
            skip_ws();
            const Position at = cursor_.position();
            if (cursor_.peek() != '"') {
                return fail(
                    std::format("expected a key in double quotes, "
                                "found {}",
                                describe_next()));
            }
            auto key = parse_basic_string(false);
            if (!key) {
                return std::unexpected(std::move(key.error()));
            }
            if (!is_valid_key(*key)) {
                return fail(std::format("unsupported key \"{}\"", *key), at);
            }
            if (object.contains(*key)) {
                return fail(std::format("duplicate key \"{}\"", *key), at);
            }
            if (auto colon = expect(':'); !colon) {
                return std::unexpected(std::move(colon.error()));
            }
            auto value = parse_value();
            if (!value) {
                return value;
            }
            object.emplace(std::move(*key), std::move(*value));
            skip_ws();
            if (cursor_.consume('}')) {
                return object;
            }
            if (!cursor_.consume(',')) {
                return fail(std::format("expected ',' or '}}', found {}",
                                        describe_next()));
            }
        }
    }

    Result<Value> parse_array() {
        cursor_.advance();
        Array array;
        skip_ws();
        if (cursor_.consume(']')) {
            return array;
        }
        while (true) {
            auto value = parse_value();
            if (!value) {
                return value;
            }
            array.push_back(std::move(*value));
            skip_ws();
            if (cursor_.consume(']')) {
                return array;
            }
            if (!cursor_.consume(',')) {
                return fail(std::format("expected ',' or ']', found {}",
                                        describe_next()));
            }
        }
    }
};

class JsonWriter final {
public:
    std::string run(const Value& root) {
        write(root, 0);
        out_ += '\n';
        return std::move(out_);
    }

private:
    void write(const Value& value, std::size_t depth) {
        value.visit(Overloaded{
            [&](std::monostate) { out_ += "null"; },
            [&](bool b) { out_ += b ? "true" : "false"; },
            [&](std::int64_t i) { out_ += format_number(i); },
            [&](double d) { out_ += format_number(d); },
            [&](const std::string& s) { out_ += quote(s); },
            [&](const Array& array) {
                write_block('[', ']', array.empty(), depth, [&] {
                    for (const char* sep = ""; const Value& item : array) {
                        out_ += sep;
                        indent(depth + 1);
                        write(item, depth + 1);
                        sep = ",\n";
                    }
                });
            },
            [&](const Object& object) {
                write_block('{', '}', object.empty(), depth, [&] {
                    for (const char* sep = "";
                         const auto& [key, item] : object) {
                        out_ += sep;
                        indent(depth + 1);
                        out_ += quote(key) + ": ";
                        write(item, depth + 1);
                        sep = ",\n";
                    }
                });
            },
        });
    }

    template <class WriteItems>
    void write_block(char open, char close, bool empty, std::size_t depth,
                     WriteItems write_items) {
        out_ += open;
        if (!empty) {
            out_ += '\n';
            write_items();
            out_ += '\n';
            indent(depth);
        }
        out_ += close;
    }

    void indent(std::size_t depth) { out_.append(depth * 2, ' '); }

    std::string out_;
};

}  // namespace

Result<Value> Json::read(std::string_view text) {
    return JsonReader(text).parse_document();
}

Result<std::string> Json::write(const Value& value) {
    return JsonWriter{}.run(value);
}

}  // namespace lab4
