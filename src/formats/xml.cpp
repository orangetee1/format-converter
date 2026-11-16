

#include <cmath>
#include <format>
#include <string>
#include <utility>

#include "core/overloaded.hpp"
#include "core/parser_base.hpp"
#include "formats/formats.hpp"

namespace lab4 {

namespace {

constexpr bool is_space(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

constexpr bool is_name_char(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || is_digit(c) ||
           c == '_' || c == '-' || c == '.' || c == ':';
}

constexpr std::pair<std::string_view, char> kEntities[] = {{"&lt;", '<'},
                                                           {"&gt;", '>'},
                                                           {"&amp;", '&'},
                                                           {"&quot;", '"'},
                                                           {"&apos;", '\''}};

struct StartTag {
    std::string name;
    std::string type;
    Position position;
    bool self_closing = false;
};

class XmlReader final : public ParserBase {
public:
    static constexpr std::string_view kFormatName = Xml::name;

    explicit XmlReader(std::string_view text) noexcept : ParserBase(text) {}

    void skip_ws() noexcept {
        while (is_space(cursor_.peek())) {
            cursor_.advance();
        }
    }

    Result<Value> parse_document() {
        if (auto text = require_text(); !text) {
            return std::unexpected(std::move(text.error()));
        }
        if (cursor_.starts_with("<?xml")) {
            if (auto prolog = skip_past("?>", "XML declaration"); !prolog) {
                return std::unexpected(std::move(prolog.error()));
            }
        }
        if (auto misc = skip_misc(); !misc) {
            return std::unexpected(std::move(misc.error()));
        }
        auto tag = parse_start_tag();
        if (!tag) {
            return std::unexpected(std::move(tag.error()));
        }
        if (tag->name != "root") {
            return fail(std::format("root element must be <root>, found <{}>",
                                    tag->name),
                        tag->position);
        }
        auto root = parse_content(*tag);
        if (!root) {
            return root;
        }
        if (auto misc = skip_misc(); !misc) {
            return std::unexpected(std::move(misc.error()));
        }
        if (!cursor_.eof()) {
            return fail(std::format("unexpected {} after the root element",
                                    describe_next()));
        }
        return root;
    }

private:
    Result<void> skip_past(std::string_view end, std::string_view what) {
        const Position start = cursor_.position();
        const std::size_t n = cursor_.rest().find(end);
        if (n == std::string_view::npos) {
            return fail(std::format("unterminated {}", what), start);
        }
        for (std::size_t i = 0; i < n + end.size(); ++i) {
            cursor_.advance();
        }
        return {};
    }

    Result<void> skip_misc() {
        while (true) {
            skip_ws();
            if (!cursor_.starts_with("<!--")) {
                return {};
            }
            if (auto comment = skip_past("-->", "comment"); !comment) {
                return comment;
            }
        }
    }

    std::string parse_name() {
        const std::size_t from = cursor_.offset();
        while (is_name_char(cursor_.peek())) {
            cursor_.advance();
        }
        return std::string(cursor_.slice_from(from));
    }

    Result<StartTag> parse_start_tag() {
        StartTag tag;
        tag.position = cursor_.position();
        if (!cursor_.consume('<')) {
            return fail(std::format("expected '<', found {}", describe_next()));
        }
        tag.name = parse_name();
        if (tag.name.empty()) {
            return fail(std::format("expected an element name, found {}",
                                    describe_next()));
        }
        while (true) {
            skip_ws();
            if (cursor_.consume("/>")) {
                tag.self_closing = true;
                break;
            }
            if (cursor_.consume('>')) {
                break;
            }
            const Position at = cursor_.position();
            const std::string attribute = parse_name();
            if (attribute != "type" || !tag.type.empty()) {
                return fail(
                    attribute.empty()
                        ? std::format("expected '>', found {}", describe_next())
                        : std::format("unsupported attribute '{}': "
                                      "only one 'type' is allowed",
                                      attribute),
                    at);
            }
            if (auto eq = expect('='); !eq) {
                return std::unexpected(std::move(eq.error()));
            }
            skip_ws();
            const char delimiter = cursor_.peek();
            if (!cursor_.consume('"') && !cursor_.consume('\'')) {
                return fail(std::format("expected a quoted value, found {}",
                                        describe_next()));
            }
            const std::size_t from = cursor_.offset();
            while (!cursor_.eof() && cursor_.peek() != delimiter) {
                cursor_.advance();
            }
            tag.type = cursor_.slice_from(from);
            if (!cursor_.consume(delimiter)) {
                return fail("unterminated attribute value", at);
            }
        }
        if (tag.type.empty()) {
            return fail(
                std::format("element <{}> has no type attribute", tag.name),
                tag.position);
        }
        return tag;
    }

    Result<void> parse_end_tag(const StartTag& tag) {
        const Position at = cursor_.position();
        cursor_.consume("</");
        const std::string name = parse_name();
        skip_ws();
        if (name != tag.name || !cursor_.consume('>')) {
            return fail(
                std::format("expected </{}> to close the element "
                            "opened at {}:{}",
                            tag.name, tag.position.line, tag.position.column),
                at);
        }
        return {};
    }

    Result<Value> parse_content(const StartTag& tag) {
        if (tag.type == "object" || tag.type == "array") {
            return parse_container(tag);
        }
        if (tag.type == "string" || tag.type == "int" || tag.type == "float" ||
            tag.type == "bool") {
            return parse_scalar(tag);
        }
        return fail(std::format("unknown type \"{}\"", tag.type), tag.position);
    }

    Result<Value> parse_container(const StartTag& parent) {
        const bool is_array = parent.type == "array";
        Array array;
        Object object;
        while (!parent.self_closing) {
            if (auto misc = skip_misc(); !misc) {
                return std::unexpected(std::move(misc.error()));
            }
            if (cursor_.starts_with("</")) {
                if (auto end = parse_end_tag(parent); !end) {
                    return std::unexpected(std::move(end.error()));
                }
                break;
            }
            if (cursor_.eof()) {
                return fail(std::format("unclosed element <{}>", parent.name),
                            parent.position);
            }
            if (cursor_.peek() != '<') {
                return fail(
                    std::format("text is not allowed inside <{}> of "
                                "type \"{}\"",
                                parent.name, parent.type));
            }
            auto child = parse_start_tag();
            if (!child) {
                return std::unexpected(std::move(child.error()));
            }
            const std::string& name = child->name;
            if (is_array ? name != "item" : !is_valid_key(name)) {
                return fail(is_array
                                ? std::format("array elements must be "
                                              "<item>, found <{}>",
                                              name)
                                : std::format("unsupported key \"{}\"", name),
                            child->position);
            }
            if (!is_array && object.contains(name)) {
                return fail(std::format("duplicate key \"{}\"", name),
                            child->position);
            }
            auto value = parse_content(*child);
            if (!value) {
                return value;
            }
            if (is_array) {
                array.push_back(std::move(*value));
            } else {
                object.emplace(name, std::move(*value));
            }
        }
        if (is_array) {
            return array;
        }
        return object;
    }

    bool parse_entity(std::string& out) {
        for (const auto& [entity, c] : kEntities) {
            if (cursor_.consume(entity)) {
                out += c;
                return true;
            }
        }
        return false;
    }

    Result<Value> parse_scalar(const StartTag& tag) {
        const Position at = cursor_.position();
        std::string text;
        while (!tag.self_closing && !cursor_.starts_with("</")) {
            if (cursor_.eof()) {
                return fail(std::format("unclosed element <{}>", tag.name),
                            tag.position);
            }
            if (cursor_.peek() == '<') {
                return fail(
                    std::format("element <{}> of type \"{}\" cannot "
                                "contain elements",
                                tag.name, tag.type));
            }
            if (cursor_.peek() != '&') {
                text += cursor_.advance();
                continue;
            }
            if (!parse_entity(text)) {
                return fail(
                    "unknown entity: expected &lt; &gt; &amp; &quot; "
                    "or &apos;");
            }
        }
        if (!tag.self_closing) {
            if (auto end = parse_end_tag(tag); !end) {
                return std::unexpected(std::move(end.error()));
            }
        }

        if (tag.type == "string") {
            return Value(std::move(text));
        }
        if (tag.type == "bool" && (text == "true" || text == "false")) {
            return Value(text == "true");
        }
        if (tag.type == "int") {
            if (const auto number = parse_number<std::int64_t>(text)) {
                return Value(*number);
            }
        }
        if (tag.type == "float") {
            if (const auto number = parse_number<double>(text);
                number && std::isfinite(*number)) {
                return Value(*number);
            }
        }
        return fail(std::format("invalid {} value \"{}\"", tag.type, text), at);
    }
};

class XmlWriter final {
public:
    std::string run(const Value& root) {
        out_ = "<?xml version=\"1.0\"?>\n";
        write_element("root", root, 0);
        return std::move(out_);
    }

private:
    void write_element(std::string_view name, const Value& value,
                       std::size_t depth) {
        out_.append(depth * 2, ' ');
        out_ += std::format("<{} type=\"{}\"", name, value.type_name());
        const Array* array = value.get_if<Array>();
        const Object* object = value.get_if<Object>();
        if ((array != nullptr && !array->empty()) ||
            (object != nullptr && !object->empty())) {
            out_ += ">\n";
            if (array != nullptr) {
                for (const Value& item : *array) {
                    write_element("item", item, depth + 1);
                }
            } else {
                for (const auto& [key, item] : *object) {
                    write_element(key, item, depth + 1);
                }
            }
            out_.append(depth * 2, ' ');
            out_ += std::format("</{}>\n", name);
            return;
        }
        const std::string text = leaf_text(value);
        out_ += text.empty() ? "/>\n" : std::format(">{}</{}>\n", text, name);
    }

    static std::string leaf_text(const Value& value) {
        return value.visit(Overloaded{
            [](bool b) -> std::string { return b ? "true" : "false"; },
            [](std::int64_t i) { return format_number(i); },
            [](double d) { return format_number(d); },
            [](const std::string& s) {
                std::string out;
                for (const char c : s) {
                    switch (c) {
                        case '&':
                            out += "&amp;";
                            break;
                        case '<':
                            out += "&lt;";
                            break;
                        case '>':
                            out += "&gt;";
                            break;
                        default:
                            out += c;
                    }
                }
                return out;
            },
            [](const auto&) { return std::string(); },
        });
    }

    std::string out_;
};

}  // namespace

Result<Value> Xml::read(std::string_view text) {
    return XmlReader(text).parse_document();
}

Result<std::string> Xml::write(const Value& value) {
    if (value.contains_null()) {
        return conversion_error(name, "unsupported type: null");
    }
    return XmlWriter{}.run(value);
}

}  // namespace lab4
