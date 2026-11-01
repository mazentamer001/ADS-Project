#include "Queue.h"
#include <new>

Queue::Queue()
    : items(nullptr), capacity(5), frontIndex(0), rearIndex(0), count(0) {
    items = new int[capacity];
}

Queue::~Queue() {
    delete[] items;
}

bool Queue::resize() {
    const int newCapacity = capacity * 2;
    int* newItems = new (std::nothrow) int[newCapacity];

    if (newItems == nullptr) {
        return false;
    }

    // Copy elements in FIFO order, starting with the current front.
    for (int i = 0; i < count; ++i) {
        newItems[i] = items[(frontIndex + i) % capacity];
    }

    delete[] items;
    items = newItems;
    capacity = newCapacity;

    // The copied elements now occupy positions 0 through count - 1.
    frontIndex = 0;
    rearIndex = count;

    return true;
}

bool Queue::enqueue(int value) {
    if (count == capacity) {
        if (!resize()) {
            return false;
        }
    }

    items[rearIndex] = value;
    rearIndex = (rearIndex + 1) % capacity;
    ++count;

    return true;
}

bool Queue::dequeue() {
    if (isEmpty()) {
        return false;
    }

    frontIndex = (frontIndex + 1) % capacity;
    --count;

    return true;
}

bool Queue::front(int& value) const {
    if (isEmpty()) {
        return false;
    }

    value = items[frontIndex];
    return true;
}

bool Queue::isEmpty() const {
    return count == 0;
}

int Queue::size() const {
    return count;
}

int Queue::getCapacity() const {
    return capacity;
}
