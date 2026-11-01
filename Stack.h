#ifndef STACK_H
#define STACK_H

class Stack {
private:
    int* items;
    int capacity;
    int topIndex;

    bool resize();

public:
    Stack();
    ~Stack();

    // This simple implementation does not support copying.
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    bool push(int value);
    bool pop();
    bool top(int& value) const;

    bool isEmpty() const;
    int size() const;
    int getCapacity() const;
};

#endif
