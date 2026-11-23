#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H
#include <iostream>

//its a min heap on an array so the event with the smallest time is always at the top, if two events have the same time, the one pushed first comes out first
//the capacity is given when the queue is made and never changes, the user of the queue works out how many events can be waiting at once

//one event means at time "time", net "netId" changes to "val"
//its a class so the fields can only be set through the constructor and read through the getters
class Event {
private:
    int time; //when the change happens
    int seq; //the order it was pushed in, used when the times are equal
    int netId; //which net changes, getting the number from the hash map
    char val; //new value '0' '1' 'X' or 'Z'

public:
    //the default values are needed so the queue can make an array of events with new Event[capacity]
    Event(int t = -1, int s = -1, int n = -1, char v = 'X') {
        time = t;
        seq = s;
        netId = n;
        val = v;
    }

    int getTime() const { return time; }
    int getSeq() const { return seq; }
    int getNetId() const { return netId; }
    char getVal() const { return val; }
};

class EventQueue {
private:
    Event* heap; //heap[0] is always the next event
    int count; //number of events in the heap
    int cap; //size of the array, fixed when the queue is made
    int nextSeq; //next seq number to give out

    //true if a should come out before b, the smaller time first, and if the times are the same then smaller seq first
    bool isBefore(const Event& a, const Event& b) const {
        if (a.getTime() != b.getTime())
            return a.getTime() < b.getTime();
        return a.getSeq() < b.getSeq();
    }

    void swapSpots(int i, int j) {
        Event temp = heap[i];
        heap[i] = heap[j];
        heap[j] = temp;
    }

    //keep swapping with the parent while we come before it, parent of i is (i-1)/2
    void siftUp(int i) {
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (isBefore(heap[i], heap[parent])) {
                swapSpots(i, parent);
                i = parent;
            } else {
                break;
            }
        }
    }

    //keep swapping with the smaller child until both children come after it, children of i are 2i+1 and 2i+2
    void siftDown(int i) {
        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;

            if (left < count && isBefore(heap[left], heap[smallest]))
                smallest = left;
            if (right < count && isBefore(heap[right], heap[smallest]))
                smallest = right;

            if (smallest == i)
                break;

            swapSpots(i, smallest);
            i = smallest;
        }
    }

public:
    //makes the array once with room for exactly "capacity" events
    EventQueue(int capacity) {
        if (capacity < 1)
            capacity = 1;
        cap = capacity;
        heap = new Event[cap];
        count = 0;
        nextSeq = 0;
    }

    ~EventQueue() {
        delete[] heap;
    }

    //copying would make two queues share one array and delete it twice, so its turned off
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;

    //add the event at the end then move it up to where it belongs
    //returns false (and adds nothing) if the queue is already full
    bool push(int time, int netId, char val) {
        if (count == cap) {
            std::cout << "error: event queue is full (capacity " << cap << ")" << std::endl;
            return false;
        }

        heap[count] = Event(time, nextSeq, netId, val);
        nextSeq++;
        count++;
        siftUp(count - 1);
        return true;
    }

    //take the top event and put the last one at the top and move it down, on an empty queue it prints an error and returns an event with time -1
    Event pop() {
        if (count == 0) {
            std::cout << "error: pop on empty event queue" << std::endl;
            return Event(); //all fields -1 and 'X'
        }

        Event first = heap[0];
        count--;
        if (count > 0) {
            heap[0] = heap[count];
            siftDown(0);
        }
        return first;
    }

    //next event without removing it
    Event peek() const {
        if (count == 0) {
            std::cout << "error: peek on empty event queue" << std::endl;
            return Event();
        }
        return heap[0];
    }

    //time of the next event, -1 if empty
    int nextTime() const {
        if (count == 0)
            return -1;
        return heap[0].getTime();
    }

    bool isEmpty() const {
        return count == 0;
    }

    bool isFull() const {
        return count == cap;
    }

    int size() const {
        return count;
    }

    int capacity() const {
        return cap;
    }

    //reset so it can be used again, the array stays allocated
    void clear() {
        count = 0;
        nextSeq = 0;
    }
};

//design decisions:
//used a heap and not a sorted list cause inserting in a sorted list is O(n), the heap does push and pop in O(log k)
//a heap alone doesnt keep equal times in order, so seq is used to break the ties and get fifo
//the capacity is fixed: the array is made once in the constructor and never copied or resized.
//the simulator knows how many events can be waiting at the same time, so it passes that number in.
//if push is called on a full queue it returns false instead of writing past the array
//Event is a class with private fields so an event cant be changed after its made, only read

//complexity:
//push O(log k), pop O(log k)
//peek, nextTime, isEmpty, isFull, size, capacity, clear O(1)
//k events in a whole simulation take O(k log k)
//space O(capacity)
#endif
