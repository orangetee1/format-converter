#pragma once

// Абстрактное дерево документа — общий язык для JSON, TOML и XML.

#include <concepts>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace lab4 {

class Value;

using Array = std::vector<Value>;
using Object = std::map<std::string, Value>;

using Data = std::variant<std::monostate, bool, std::int64_t, double,
                          std::string, Array, Object>;

class Value final {
public:
    Value() = default;  // null

    template <class T>
        requires(!std::same_as<std::remove_cvref_t<T>, Value> &&
                 std::constructible_from<Data, T>)
    Value(T&& value) : data_(std::forward<T>(value)) {}

    template <class T>
    [[nodiscard]] bool is() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    template <class T, class Self>
    [[nodiscard]] decltype(auto) get(this Self&& self) {
        return std::get<T>(std::forward<Self>(self).data_);
    }

    template <class T, class Self>
    [[nodiscard]] auto get_if(this Self& self) noexcept {
        return std::get_if<T>(&self.data_);
    }

    template <class Self, class F>
    decltype(auto) visit(this Self&& self, F&& f) {
        return std::visit(std::forward<F>(f), std::forward<Self>(self).data_);
    }

    [[nodiscard]] std::string_view type_name() const noexcept {
        constexpr std::string_view kNames[] = {
            "null", "bool", "int", "float", "string", "array", "object"};
        return kNames[data_.index()];
    }

    [[nodiscard]] bool contains_null() const;

    friend bool operator==(const Value&, const Value&) = default;

private:
    Data data_;
};

[[nodiscard]] constexpr bool is_valid_key(std::string_view key) noexcept {
    const auto is_letter = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    };
    if (key.empty() || !is_letter(key.front())) {
        return false;
    }
    for (const char c : key) {
        if (!is_letter(c) && !(c >= '0' && c <= '9') && c != '-') {
            return false;
        }
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const Value& value);

}  // namespace lab4
