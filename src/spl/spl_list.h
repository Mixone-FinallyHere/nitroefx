#pragma once

#include <type_traits>

#include "types.h"


template<class T>
struct SPLNode {
    T* next;
    T* prev;
};

template<class T, std::enable_if_t<std::is_base_of_v<SPLNode<T>, T>, bool> = true>
struct SPLListIterator {
    T* ptr;

    SPLListIterator& operator++() {
        ptr = ptr->next;
        return *this;
    }

    SPLListIterator operator++(int) {
        auto copy = *this;
        ptr = ptr->next;
        return copy;
    }

    SPLListIterator& operator--() {
        ptr = ptr->prev;
        return *this;
    }

    SPLListIterator operator--(int) {
        auto copy = *this;
        ptr = ptr->prev;
        return copy;
    }

    T& operator*() { return *ptr; }
    T* operator->() { return ptr; }

    bool operator==(const SPLListIterator& other) const { return ptr == other.ptr; }
    bool operator!=(const SPLListIterator& other) const { return ptr != other.ptr; }

    SPLListIterator(T* ptr) : ptr(ptr) {}
    SPLListIterator() : ptr(nullptr) {}
};

template<class T>
struct SPLList {
    using iterator = SPLListIterator<T>;
    using const_iterator = SPLListIterator<const T>;

    T* first;
    u32 count;
    T* last;

    iterator begin() { return first; }
    iterator end() { return nullptr; }

    const_iterator begin() const { return first; }
    const_iterator end() const { return nullptr; }
};
