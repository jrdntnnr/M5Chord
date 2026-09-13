#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>

namespace midibrain {

template <typename T, std::size_t Capacity>
class FixedList {
public:
    constexpr std::size_t size() const { return size_; }
    constexpr std::size_t capacity() const { return Capacity; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr bool full() const { return size_ == Capacity; }

    bool push_back(const T& value) {
        if (full()) {
            return false;
        }
        values_[size_++] = value;
        return true;
    }

    bool push_back(T&& value) {
        if (full()) {
            return false;
        }
        values_[size_++] = std::move(value);
        return true;
    }

    void clear() { size_ = 0; }

    T& operator[](std::size_t index) { return values_[index]; }
    const T& operator[](std::size_t index) const { return values_[index]; }
    T* begin() { return values_.data(); }
    T* end() { return values_.data() + size_; }
    const T* begin() const { return values_.data(); }
    const T* end() const { return values_.data() + size_; }

    void erase(std::size_t index) {
        if (index >= size_) {
            return;
        }
        for (std::size_t i = index + 1; i < size_; ++i) {
            values_[i - 1] = std::move(values_[i]);
        }
        --size_;
    }

    template <typename Predicate>
    std::size_t erase_if(Predicate predicate) {
        std::size_t write = 0;
        for (std::size_t read = 0; read < size_; ++read) {
            if (!predicate(values_[read])) {
                values_[write++] = std::move(values_[read]);
            }
        }
        const std::size_t removed = size_ - write;
        size_ = write;
        return removed;
    }

    template <typename Compare>
    void sort(Compare compare) {
        std::sort(begin(), end(), compare);
    }

private:
    std::array<T, Capacity> values_{};
    std::size_t size_{0};
};

}

