#pragma once

namespace lab4 {

template <class... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};

}  // namespace lab4
