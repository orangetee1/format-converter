#include <cstdio>
#include <iostream>
#include <iterator>
#include <print>
#include <string>

#include "core/error.hpp"
#include "formats/formats.hpp"

int main(int argc, char* argv[]) {
    using namespace lab4;

    const auto known = [](const char* name) {
        return dispatch(name, [](Format auto) {});
    };
    if (argc != 3 || !known(argv[1]) || !known(argv[2])) {
        std::println(stderr, "usage: lab4 <json|toml|xml> <json|toml|xml>");
        return 1;
    }

    const std::string text{std::istreambuf_iterator<char>(std::cin), {}};

    Result<std::string> result;
    dispatch(argv[1], [&]<Format In>(In) {
        dispatch(argv[2],
                 [&]<Format Out>(Out) { result = convert<In, Out>(text); });
    });
    if (!result) {
        std::print(stderr, "{}", render_error(result.error(), text));

        return result.error().line != 0 ? 2 : 3;
    }
    std::print("{}", *result);
    return 0;
}
