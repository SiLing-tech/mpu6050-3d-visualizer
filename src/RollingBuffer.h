#pragma once

#include <array>

template <typename T, int Capacity>
class RollingBuffer {
    std::array<T, Capacity> buf_{};
    int head_ = 0;
    int count_ = 0;

public:
    void push(const T &v)
    {
        buf_[head_] = v;
        head_ = (head_ + 1) % Capacity;
        if (count_ < Capacity)
            count_++;
    }

    const T &operator[](int i) const { return buf_[(head_ - count_ + i + Capacity) % Capacity]; }

    int size() const { return count_; }
    int capacity() const { return Capacity; }
    bool isFull() const { return count_ == Capacity; }
};
