#pragma once

#include <assert.h>

namespace zw {

template<typename Wrapped>
class Option {
    union {
        Wrapped _object;
    };
    bool _is_present;
public:
    Option() {
        _is_present = false;
    }

    Option(Wrapped&& object) {
        new(&_object) Wrapped(std::move(object));
        _is_present = true;
    }

    Option(Option<Wrapped>&& other) {
        _is_present = other._is_present;
        if(other._is_present) {
            new(&_object) Wrapped(std::move(other._object));
            other._object.~Wrapped();
            other._is_present = false;
        }
    }

    Option(const Option<Wrapped>& other) {
        _is_present = other._is_present;
        if(other._is_present) {
            new(&_object) Wrapped(other._object);
        }
    }

    Option& operator=(Option<Wrapped>&& other) {
        if(this != &other) {
            if(_is_present) {
                _object.~Wrapped();
            }
            _is_present = other._is_present;
            if(other._is_present) {
                new(&_object) Wrapped(std::move(other._object));
                other._is_present = false;
                other._object.~Wrapped();
            }
        }
        return *this;
    }

    Option& operator=(const Option<Wrapped>& other) {
        if(this != &other) {
            if(_is_present) {
                _object.~Wrapped();
            }
            _is_present = other._is_present;
            new(&_object) Wrapped(other._object);
        }
        return *this;
    }

    ~Option() {
        if(_is_present) {
            _object.~Wrapped();
            _is_present = false;
        }
    }

    bool is_present() const { return _is_present; }
    Wrapped const& unwrap() const {
        assert(_is_present);
        return _object;
    }
    Wrapped& unwrap() {
        assert(_is_present);
        return _object;
    }

    /// Just like unwrap(), except it moves the object out of the Option
    /// Does not run destructor on moved-from object
    Wrapped take() {
        assert(_is_present);
        _is_present = false;
        return std::move(_object);
    }
};

template<typename Wrapped>
Option<Wrapped> none() { return {}; }

};