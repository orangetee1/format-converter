#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <vector>

namespace lab4 {

template <class Key, class Mapped>
class OrderedMap final {
public:
    using value_type = std::pair<Key, Mapped>;
    using iterator = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    OrderedMap() = default;

    OrderedMap(std::initializer_list<value_type> init) {
        for (const value_type& entry : init) {
            insert(entry.first, entry.second);
        }
    }

    bool insert(Key key, Mapped value) {
        if (contains(key)) {
            return false;
        }
        entries_.emplace_back(std::move(key), std::move(value));
        return true;
    }

    [[nodiscard]] Mapped* find(const Key& key) {
        const std::size_t i = index_of(key);
        return i < entries_.size() ? &entries_[i].second : nullptr;
    }
    [[nodiscard]] const Mapped* find(const Key& key) const {
        const std::size_t i = index_of(key);
        return i < entries_.size() ? &entries_[i].second : nullptr;
    }

    [[nodiscard]] bool contains(const Key& key) const {
        return index_of(key) < entries_.size();
    }

    [[nodiscard]] const Mapped& at(const Key& key) const {
        if (const Mapped* value = find(key)) {
            return *value;
        }
        throw std::out_of_range("OrderedMap::at: key not found");
    }

    [[nodiscard]] iterator begin() noexcept { return entries_.begin(); }
    [[nodiscard]] iterator end() noexcept { return entries_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept {
        return entries_.begin();
    }
    [[nodiscard]] const_iterator end() const noexcept { return entries_.end(); }

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

    friend bool operator==(const OrderedMap& lhs, const OrderedMap& rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        return std::ranges::all_of(lhs.entries_, [&rhs](const value_type& e) {
            const Mapped* other = rhs.find(e.first);
            return other != nullptr && *other == e.second;
        });
    }

private:
    [[nodiscard]] std::size_t index_of(const Key& key) const {
        const auto it = std::ranges::find(entries_, key, &value_type::first);
        return static_cast<std::size_t>(it - entries_.begin());
    }

    std::vector<value_type> entries_;
};

}  // namespace lab4
