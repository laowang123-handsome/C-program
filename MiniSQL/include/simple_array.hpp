#ifndef MINISQL_SIMPLE_ARRAY_HPP
#define MINISQL_SIMPLE_ARRAY_HPP

#include <cstddef>
#include <stdexcept>

// A tiny self-made dynamic array used instead of STL containers.
// It intentionally avoids std::vector/list/map/set and all container adapters.
template <typename T>
class SimpleArray {
private:
    T* data_;
    std::size_t size_;
    std::size_t capacity_;

    void reallocate(std::size_t new_capacity) {
        T* new_data = new T[new_capacity];
        for (std::size_t i = 0; i < size_; ++i) {
            new_data[i] = data_[i];
        }
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }

public:
    SimpleArray() : data_(nullptr), size_(0), capacity_(0) {}

    explicit SimpleArray(std::size_t initial_capacity)
        : data_(nullptr), size_(0), capacity_(0) {
        if (initial_capacity > 0) {
            data_ = new T[initial_capacity];
            capacity_ = initial_capacity;
        }
    }

    SimpleArray(const SimpleArray& other)
        : data_(nullptr), size_(0), capacity_(0) {
        if (other.capacity_ > 0) {
            data_ = new T[other.capacity_];
            capacity_ = other.capacity_;
            size_ = other.size_;
            for (std::size_t i = 0; i < size_; ++i) {
                data_[i] = other.data_[i];
            }
        }
    }

    SimpleArray& operator=(const SimpleArray& other) {
        if (this == &other) {
            return *this;
        }
        delete[] data_;
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        if (other.capacity_ > 0) {
            data_ = new T[other.capacity_];
            capacity_ = other.capacity_;
            size_ = other.size_;
            for (std::size_t i = 0; i < size_; ++i) {
                data_[i] = other.data_[i];
            }
        }
        return *this;
    }

    ~SimpleArray() {
        delete[] data_;
    }

    void push_back(const T& value) {
        if (size_ == capacity_) {
            std::size_t next_capacity = (capacity_ == 0) ? 4 : capacity_ * 2;
            reallocate(next_capacity);
        }
        data_[size_++] = value;
    }

    void pop_back() {
        if (size_ == 0) {
            throw std::out_of_range("pop_back on empty SimpleArray");
        }
        --size_;
    }

    void clear() {
        size_ = 0;
    }

    void reserve(std::size_t n) {
        if (n > capacity_) {
            reallocate(n);
        }
    }

    void resize(std::size_t n) {
        if (n > capacity_) {
            reallocate(n);
        }
        size_ = n;
    }

    void remove_at(std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("SimpleArray index out of range");
        }
        for (std::size_t i = index + 1; i < size_; ++i) {
            data_[i - 1] = data_[i];
        }
        --size_;
    }

    T& operator[](std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("SimpleArray index out of range");
        }
        return data_[index];
    }

    const T& operator[](std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("SimpleArray index out of range");
        }
        return data_[index];
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
};

#endif
