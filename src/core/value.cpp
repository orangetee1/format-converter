#include "core/value.hpp"

#include <algorithm>
#include <ostream>
#include <ranges>

#include "core/overloaded.hpp"
#include "core/text.hpp"

namespace lab4 {

bool Value::contains_null() const {
    return visit(Overloaded{
        [](std::monostate) { return true; },
        [](const Array& array) {
            return std::ranges::any_of(array, &Value::contains_null);
        },
        [](const Object& object) {
            return std::ranges::any_of(object | std::views::values,
                                       &Value::contains_null);
        },
        [](const auto&) { return false; },
    });
}

std::ostream& operator<<(std::ostream& os, const Value& value) {
    value.visit(Overloaded{
        [&](std::monostate) { os << "null"; },
        [&](bool b) { os << (b ? "true" : "false"); },
        [&](std::int64_t i) { os << i; },
        [&](double d) { os << format_number(d); },
        [&](const std::string& s) { os << '"' << s << '"'; },
        [&](const Array& array) {
            os << '[';
            for (const char* sep = ""; const Value& item : array) {
                os << sep << item;
                sep = ", ";
            }
            os << ']';
        },
        [&](const Object& object) {
            os << '{';
            for (const char* sep = ""; const auto& [key, item] : object) {
                os << sep << '"' << key << "\": " << item;
                sep = ", ";
            }
            os << '}';
        },
    });
    return os;
}

}  // namespace lab4
