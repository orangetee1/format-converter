// Тесты Error и наглядного вывода render_error.

#include "core/error.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <expected>
#include <sstream>
#include <string>
#include <string_view>

namespace {

using lab4::Error;
using lab4::ErrorKind;
using lab4::Position;
using lab4::render_error;

}  // namespace

TEST(ErrorKind, Names) {
    EXPECT_EQ(lab4::to_string(ErrorKind::Syntax), "syntax");
    EXPECT_EQ(lab4::to_string(ErrorKind::Unsupported), "unsupported");
    EXPECT_EQ(lab4::to_string(ErrorKind::Semantic), "semantic");
    EXPECT_EQ(lab4::to_string(ErrorKind::Limit), "limit");
    EXPECT_EQ(lab4::to_string(ErrorKind::Io), "io");
    // to_string — constexpr, значит, работает и в static_assert.
    static_assert(lab4::to_string(ErrorKind::Syntax) == "syntax");
}

TEST(Error, DesignatedInitializersAndEquality) {
    const Error a{.kind = ErrorKind::Semantic,
                  .format = "toml",
                  .message = "duplicate key 'a'",
                  .position = Position{.line = 2, .column = 1}};
    Error b = a;
    EXPECT_EQ(a, b);
    b.position.reset();
    EXPECT_NE(a, b);
}

TEST(Error, ResultAliasCarriesError) {
    // std::unexpected — обёртка «это ошибка, а не значение». Через CTAD
    // она выводится как std::unexpected<Error>.
    // https://en.cppreference.com/w/cpp/utility/expected/unexpected
    const lab4::Result<int> r = std::unexpected(
        Error{.kind = ErrorKind::Io, .message = "cannot read stdin"});
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().kind, ErrorKind::Io);
}

// Образец конвейера на Result<T>, по которому будут устроены парсеры и CLI.
// Это пример использования, а не тест стандартной библиотеки.
namespace {

// «Парсер» неотрицательного числа: ошибка Syntax с позицией первой
// неподходящей цифры.
lab4::Result<int> parse_digits(std::string_view text) {
    int value = 0;
    std::size_t column = 1;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return std::unexpected(Error{.kind = ErrorKind::Syntax,
                                         .message = "expected a digit",
                                         .position = Position{1, column}});
        }
        value = value * 10 + (c - '0');
        ++column;
    }
    return value;
}

// Проверка смысла: ошибка Unsupported без позиции.
lab4::Result<int> reject_zero(int value) {
    if (value == 0) {
        return std::unexpected(Error{.kind = ErrorKind::Unsupported,
                                     .message = "zero is not supported"});
    }
    return value;
}

// transform_error: дописываем, к какому формату относится ошибка.
Error with_format(Error error) {
    error.format = "demo";
    return error;
}

}  // namespace

TEST(ResultPipeline, ValueFlowsThroughAllSteps) {
    const lab4::Result<std::string> r =
        parse_digits("21")
            .and_then(reject_zero)                   // Result<int>
            .transform([](int v) { return v * 2; })  // Result<int>
            .transform(
                [](int v) { return std::to_string(v); })  // Result<string>
            .transform_error(with_format);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, "42");
}

TEST(ResultPipeline, FirstErrorSkipsRemainingSteps) {
    int later_steps = 0;
    const lab4::Result<int> r = parse_digits("4x2")
                                    .and_then([&](int v) -> lab4::Result<int> {
                                        ++later_steps;
                                        return v;
                                    })
                                    .transform([&](int v) {
                                        ++later_steps;
                                        return v;
                                    })
                                    .transform_error(with_format);
    EXPECT_EQ(later_steps, 0);  // после ошибки ни один шаг не вызван
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), (Error{.kind = ErrorKind::Syntax,
                                .format = "demo",
                                .message = "expected a digit",
                                .position = Position{1, 2}}));
}

TEST(ResultPipeline, ErrorFromLaterStep) {
    const lab4::Result<int> r =
        parse_digits("0").and_then(reject_zero).transform_error(with_format);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().kind, ErrorKind::Unsupported);
    EXPECT_EQ(r.error().format, "demo");
    EXPECT_FALSE(r.error().position.has_value());
}

TEST(Error, StreamOutput) {
    std::ostringstream os;
    os << Error{.kind = ErrorKind::Syntax,
                .format = "json",
                .message = "unexpected token",
                .position = Position{3, 15}};
    EXPECT_EQ(os.str(), "[syntax] json: unexpected token at 3:15");
}

TEST(RenderError, HeaderOnlyWithoutPosition) {
    const Error e{.kind = ErrorKind::Io, .message = "failed to read stdin"};
    EXPECT_EQ(render_error(e, "whatever"), "error[io]: failed to read stdin\n");
}

TEST(RenderError, PointsToColumnInMultilineSource) {
    const std::string source =
        "{\n"
        "  \"name\": \"Alice\",\n"
        "  \"age\": 42 \"x\"\n"
        "}\n";
    // В третьей строке перед "x" стоят 12 символов: `  "age": 42 `.
    const Error e{.kind = ErrorKind::Syntax,
                  .format = "json",
                  .message = "expected ',' or '}'",
                  .position = Position{3, 13}};
    const std::string expected =
        "error[syntax]: json: expected ',' or '}'\n"
        " --> <stdin>:3:13\n"
        "  3 |   \"age\": 42 \"x\"\n"
        "    | " +
        std::string(12, ' ') + "^\n";
    EXPECT_EQ(render_error(e, source), expected);
}

TEST(RenderError, CaretAtFirstColumnOfFirstLine) {
    const Error e{.kind = ErrorKind::Unsupported,
                  .format = "json",
                  .message = "root must be an object",
                  .position = Position{1, 1}};
    EXPECT_EQ(render_error(e, "[1, 2]"),
              "error[unsupported]: json: root must be an object\n"
              " --> <stdin>:1:1\n"
              "  1 | [1, 2]\n"
              "    | ^\n");
}

TEST(RenderError, GutterWidthFollowsLineNumber) {
    // 10 строк: номер двузначный, поэтому '|' сдвигается на 1 вправо и в
    // обеих строках стоит на одном месте.
    std::string source;
    for (int i = 1; i <= 10; ++i) {
        source += "line" + std::to_string(i) + "\n";
    }
    const Error e{.kind = ErrorKind::Syntax,
                  .message = "bad",
                  .position = Position{10, 5}};
    EXPECT_EQ(render_error(e, source),
              "error[syntax]: bad\n"
              " --> <stdin>:10:5\n"
              "  10 | line10\n"
              "     |     ^\n");
}

TEST(RenderError, TabsArePreservedInPadding) {
    // Табуляция в исходнике копируется в отступ, чтобы терминал развернул
    // её одинаково в обеих строках.
    const Error e{.kind = ErrorKind::Syntax,
                  .format = "toml",
                  .message = "unterminated string",
                  .position = Position{1, 8}};
    EXPECT_EQ(render_error(e, "\tkey = \"x"),
              "error[syntax]: toml: unterminated string\n"
              " --> <stdin>:1:8\n"
              "  1 | \tkey = \"x\n"
              "    | \t      ^\n");
}

TEST(RenderError, ColumnsCountUtf8CodePoints) {
    // "привет" — 6 символов, но 12 байт. x стоит в 14-й колонке, отступ —
    // 13 пробелов, а не 19.
    const std::string line = "a = \"привет\" x";
    const Error e{.kind = ErrorKind::Syntax,
                  .message = "unexpected 'x'",
                  .position = Position{1, 14}};
    EXPECT_EQ(render_error(e, line),
              "error[syntax]: unexpected 'x'\n"
              " --> <stdin>:1:14\n"
              "  1 | " +
                  line +
                  "\n"
                  "    | " +
                  std::string(13, ' ') + "^\n");
}

TEST(RenderError, CrlfLineEndingIsStripped) {
    const Error e{.kind = ErrorKind::Syntax,
                  .message = "bad",
                  .position = Position{2, 1}};
    EXPECT_EQ(render_error(e, "ok\r\nbad\r\nnext\r\n"),
              "error[syntax]: bad\n"
              " --> <stdin>:2:1\n"
              "  2 | bad\n"
              "    | ^\n");
}

TEST(RenderError, CaretPastEndOfLine) {
    // Ошибка «ожидалась '}'» указывает на позицию сразу после текста.
    const Error e{.kind = ErrorKind::Syntax,
                  .message = "expected '}'",
                  .position = Position{1, 5}};
    EXPECT_EQ(render_error(e, "{\"a\""),
              "error[syntax]: expected '}'\n"
              " --> <stdin>:1:5\n"
              "  1 | {\"a\"\n"
              "    |     ^\n");
}

TEST(RenderError, EndOfInputAfterTrailingNewline) {
    // Конец входа после финального '\n' — это начало пустой строки 2.
    const Error e{.kind = ErrorKind::Syntax,
                  .message = "unexpected end of input",
                  .position = Position{2, 1}};
    EXPECT_EQ(render_error(e, "{\n"),
              "error[syntax]: unexpected end of input\n"
              " --> <stdin>:2:1\n"
              "  2 | \n"
              "    | ^\n");
}

TEST(RenderError, LineOutOfRangeShowsOnlyLocation) {
    const Error e{.kind = ErrorKind::Syntax,
                  .message = "bad",
                  .position = Position{5, 1}};
    EXPECT_EQ(render_error(e, "one\ntwo"),
              "error[syntax]: bad\n"
              " --> <stdin>:5:1\n");
}

TEST(RenderError, CustomSourceName) {
    const Error e{.kind = ErrorKind::Syntax,
                  .message = "bad",
                  .position = Position{1, 2}};
    EXPECT_EQ(render_error(e, "xy", "input.json"),
              "error[syntax]: bad\n"
              " --> input.json:1:2\n"
              "  1 | xy\n"
              "    |  ^\n");
}
