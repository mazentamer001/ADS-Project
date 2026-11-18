#ifndef QUEUE_H
#define QUEUE_H

class Queue {
private:
    int* items;
    int capacity;
    int frontIndex;
    int rearIndex;
    int count;

    bool resize();

public:
    Queue();
    ~Queue();

    // This simple implementation does not support copying.
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    bool enqueue(int value);
    bool dequeue();
    bool front(int& value) const;

    bool isEmpty() const;
    int size() const;
    int getCapacity() const;
};

#endif
