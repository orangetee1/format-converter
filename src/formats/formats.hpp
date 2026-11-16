#pragma once

// Форматы JSON, TOML, XML и выбор формата по имени из командной строки.

#include <concepts>
#include <string>
#include <string_view>

#include "core/error.hpp"
#include "core/value.hpp"

namespace lab4 {

struct Json {
    static constexpr std::string_view name = "json";
    static Result<Value> read(std::string_view text);
    static Result<std::string> write(const Value& value);
};

struct Toml {
    static constexpr std::string_view name = "toml";
    static Result<Value> read(std::string_view text);
    static Result<std::string> write(const Value& value);
};

struct Xml {
    static constexpr std::string_view name = "xml";
    static Result<Value> read(std::string_view text);
    static Result<std::string> write(const Value& value);
};

template <class F>
concept Format = requires(std::string_view text, const Value& value) {
    { F::name } -> std::convertible_to<std::string_view>;
    { F::read(text) } -> std::same_as<Result<Value>>;
    { F::write(value) } -> std::same_as<Result<std::string>>;
};

template <Format... Fs>
struct TypeList {};

using Formats = TypeList<Json, Toml, Xml>;

template <class F>
bool dispatch(std::string_view name, F&& f) {
    return [&]<Format... Fs>(TypeList<Fs...> /*formats*/) {
        return ((name == Fs::name && (f(Fs{}), true)) || ...);
    }(Formats{});
}

template <Format In, Format Out>
Result<std::string> convert(std::string_view text) {
    return In::read(text).and_then(Out::write);
}

}  // namespace lab4
