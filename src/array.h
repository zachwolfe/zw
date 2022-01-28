#pragma once

#include <utility>
#include <assert.h>
#include "alloc.h"

namespace zw {

template<typename T>
class ConstArrayIterator {
    const T* loc;

public:
    ConstArrayIterator(const T* loc) : loc(loc) {}

    ConstArrayIterator<T>& operator++() {
        ++loc;
        return *this;
    }

    bool operator==(const ConstArrayIterator<T>& other) { return loc == other.loc; }
    bool operator!=(const ConstArrayIterator<T>& other) { return loc != other.loc; }
    const T& operator*() { return *loc; }
};

template<typename T>
class ConstArrayIterable {
    const T* data;
    size_t size;

public:
    ConstArrayIterable(const T* data, size_t size) : data(data), size(size) {}
    ConstArrayIterator<T> begin() { return data; }
    ConstArrayIterator<T> end() { return data + size; }
};

template<typename T>
class MutArrayIterator {
    T* loc;

public:
    MutArrayIterator(T* loc) : loc(loc) {}

    MutArrayIterator<T>& operator++() {
        ++loc;
        return *this;
    }

    bool operator==(const MutArrayIterator<T>& other) { return loc == other.loc; }
    bool operator!=(const MutArrayIterator<T>& other) { return loc != other.loc; }
    T& operator*() { return *loc; }
};

template<typename T>
class MutArrayIterable {
    T* data;
    size_t size;

public:
    MutArrayIterable(T* data, size_t size) : data(data), size(size) {}
    MutArrayIterator<T> begin() { return data; }
    MutArrayIterator<T> end() { return data + size; }
};

template<typename T>
class MoveArrayIterator {
    T* loc;

public:
    MoveArrayIterator(T* loc) : loc(loc) {}

    MoveArrayIterator<T>& operator++() {
        ++loc;
        return *this;
    }

    bool operator==(const MoveArrayIterator<T>& other) { return loc == other.loc; }
    bool operator!=(const MoveArrayIterator<T>& other) { return loc != other.loc; }
    T& operator*() { return *loc; }
};

template<typename T>
class Array;

template<typename T>
class MoveArrayIterable {
    T* data;
    size_t size;

public:
    MoveArrayIterable(T* data, size_t size) : data(data), size(size) {}
    MoveArrayIterable(MoveArrayIterable<T>&& other) {
        data = other.data;
        size = other.size;

        other.data = nullptr;
        other.size = 0;
    }
    MoveArrayIterator<T> begin() { return data; }
    MoveArrayIterator<T> end() { return data + size; }
    ~MoveArrayIterable() {
        Array<T> temp(data, size, size);
    }
};

template<typename T>
class Array: ZwObject {
    T* _data = 0;
    size_t _size = 0;
    size_t _cap = 0;
    constexpr static size_t INITIAL_CAPACITY = 4;

    void make_room() {
        if(_size >= _cap) {
            _cap *= 2;
            if(!_cap) {
                _cap = INITIAL_CAPACITY;
            }
            // TODO: realloc is not safe here if T is not trivially movable!
            _data = (T*)zw_realloc(_data, sizeof(T) * _cap, alignof(T));
            assert(_data && "allocator out of memory");
        }
    }

    friend class MoveArrayIterable<T>;
    Array(T* data, size_t size, size_t cap) : _data(data), _size(size), _cap(cap) {}

    void open_gap(size_t index) {
        assert(index <= _size);
        make_room();
        for(size_t i = _size; i > index; i--) {
            new(&_data[i]) T(std::move(_data[i-1]));
        }
        _size++;
    }

public:
    Array() = default;
    Array(const Array<T>& other) : ZwObject(other) {
        reserve(other._size);
        if constexpr(std::is_trivially_copyable_v<T>) {
            memcpy(_data, other._data, sizeof(T) * other._size);
        } else {
            for(size_t i = 0; i < other._size; i++) {
                new(&_data[i]) T(other._data[i]);
            }
        }
        _size = other._size;
    }
    Array(Array<T>&& other) noexcept {
        _data = other._data;
        _size = other._size;
        _cap = other._cap;

        other._data = nullptr;
        other._size = 0;
        other._cap = 0;
    }
    Array(std::initializer_list<T> list) {
        reserve(list.size());
        for(auto it = list.begin(); it != list.end(); ++it) {
            push(*it);
        }
        _size = list.size();
    }

    Array<T>& operator=(const Array<T>& other) {
        if(this != &other) {
            Array<T> temp = other;
            swap(*this, temp);
        }
        return *this;
    }

    Array<T>& operator=(Array<T>&& other) {
        if(this != &other) {
            Array<T> temp = std::move(other);
            swap(*this, temp);
        }
        return *this;
    }

    size_t size() const {
        return _size;
    }

    size_t cap() const {
        return _cap;
    }

    T* data() {
        return _data;
    }

    const T* data() const {
        return _data;
    }

    void clear() {
        if constexpr(!std::is_trivially_destructible_v<T>) {
            for(size_t i = 0; i < _size; i++) {
                _data[i].~T();
            }
        }
        _size = 0;
    }

    void erase(size_t index) {
        assert(index < _size);
        if constexpr(!std::is_trivially_destructible_v<T>) {
            _data[index].~T();
        }
        for(size_t i = index; i < _size - 1; i++) {
            std::swap(_data[i], _data[i+1]);
        }
        _size--;
    }

    template<typename F>
    void erase_if(F should_erase) {
        size_t cursor = 0;
        size_t num_erased = 0;
        for(size_t i = 0; i < _size; i++) {
            if(should_erase(i, _data[i])) {
                num_erased++;
                if constexpr(!std::is_trivially_destructible_v<T>) {
                    _data[i].~T();
                }
            } else {
                if(cursor != i) {
                    std::swap(_data[cursor], _data[i]);
                }
                cursor++;
            }
        }
        _size -= num_erased;
    }

    void reserve(size_t min_cap) {
        if(!_cap) {
            _cap = INITIAL_CAPACITY;
        }
        // Done to avoid reallocating every time reserve is called
        while(_cap < min_cap) {
            _cap *= 2;
        }
        _data = (T*)zw_realloc(_data, sizeof(T) * _cap, alignof(T));
        assert(_data && "allocator out of memory");
    }

    void resize(size_t size) requires std::default_initializable<T> {
        if(size < _size) {
            if constexpr(!std::is_trivially_destructible_v<T>) {
                for(size_t i = size; i < _size; i++) {
                    _data[i].~T();
                }
            }
            _size = size;
        } else if(size > _size) {
            reserve(size);
            for(size_t i = _size; i < size; i++) {
                new(&_data[i]) T();
            }
            _size = size;
        }
    }

    void push(const T& element) {
        make_room();
        new(&_data[_size]) T(element);
        _size++;
    }

    void push(T&& element) {
        make_room();
        new(&_data[_size]) T(std::move(element));
        _size++;
    }

    void insert(size_t index, const T& element) {
        open_gap(index);
        new(&_data[index]) T(element);
    }

    void insert(size_t index, T&& element) {
        open_gap(index);
        new(&_data[index]) T(std::move(element));
    }

    const T& operator[](size_t index) const {
        assert(index < _size);
        return _data[index];
    }

    T& operator[](size_t index) {
        assert(index < _size);
        return _data[index];
    }

    ~Array() {
        if(_data) {
            if constexpr(!std::is_trivially_destructible_v<T>) {
                for(size_t i = 0; i < _size; i++) {
                    _data[i].~T();
                }
            }
            zw_free(_data);
        }
    }

    ConstArrayIterable<T> iter() const { return {_data, _size}; }
    MutArrayIterable<T> iter_mut() { return {_data, _size}; }
    MoveArrayIterable<T> iter_move() {
        MoveArrayIterable<T> iter {_data, _size};
        _data = nullptr;
        _size = 0;
        _cap = 0;
        return iter;
    }

    T& last() {
        assert(_size > 0);
        return _data[_size - 1];
    }

    const T& last() const {
        assert(_size > 0);
        return _data[_size - 1];
    }

    bool is_empty() const { return _size == 0; }

    // Don't call unless you know what you are doing!
    void unsafe_set_size(size_t size) {
        _size = size;
    }

    friend void swap(Array<T>& left, Array<T>& right) {
        std::swap(left._data, right._data);
        std::swap(left._size, right._size);
        std::swap(left._cap, right._cap);
    }
};

}
