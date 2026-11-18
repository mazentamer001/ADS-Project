#include "Stack.h"
#include <new>

Stack::Stack() : items(nullptr), capacity(5), topIndex(-1) {
    items = new int[capacity];
}

Stack::~Stack() {
    delete[] items;
}

bool Stack::resize() {
    const int newCapacity = capacity * 2;
    int* newItems = new (std::nothrow) int[newCapacity];

    if (newItems == nullptr) {
        return false;
    }

    for (int i = 0; i <= topIndex; ++i) {
        newItems[i] = items[i];
    }

    delete[] items;
    items = newItems;
    capacity = newCapacity;

    return true;
}

bool Stack::push(int value) {
    if (topIndex + 1 == capacity) {
        if (!resize()) {
            return false;
        }
    }

    ++topIndex;
    items[topIndex] = value;
    return true;
}

bool Stack::pop() {
    if (isEmpty()) {
        return false;
    }

    --topIndex;
    return true;
}

bool Stack::top(int& value) const {
    if (isEmpty()) {
        return false;
    }

    value = items[topIndex];
    return true;
}

bool Stack::isEmpty() const {
    return topIndex == -1;
}

int Stack::size() const {
    return topIndex + 1;
}

int Stack::getCapacity() const {
    return capacity;
}
