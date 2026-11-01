#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include <iostream>
using namespace std;

//its a min heap on an array so the event with the smallest time is always at the top, if two events have the same time, the one pushed first comes out first

#define START_CAP 16 //starting size of the array, doubles when full

//one event means at time "time", net "netId" changes to "val"
struct Event {
    int time; //when the change happens
    int seq; //the order it was pushed in, used when the times are equal
    int netId; //which net changes, getting the number from the hash map
    char val; //new value '0' '1' 'X' or 'Z'
};

class EventQueue {
private:
    Event* heap; //heap[0] is always the next event
    int count; //number of events in the heap
    int cap; //size of the array
    int nextSeq; //next seq number to give out

    //true if a should come out before b, the smaller time first, and if the times are the same then smaller seq first
    bool isBefore(Event a, Event b) {
        if (a.time != b.time)
            return a.time < b.time;
        return a.seq < b.seq;
    }

    void swapSpots(int i, int j) {
        Event temp = heap[i];
        heap[i] = heap[j];
        heap[j] = temp;
    }

    //if the array is full make a new one thats twice the size and copy everything
    void grow() {
        int newCap = cap * 2;
        Event* bigger = new Event[newCap];
        for (int i = 0; i < count; i++)
            bigger[i] = heap[i];
        delete[] heap;
        heap = bigger;
        cap = newCap;
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
    EventQueue() {
        cap = START_CAP;
        heap = new Event[cap];
        count = 0;
        nextSeq = 0;
    }

    ~EventQueue() {
        delete[] heap;
    }

    //add the event at the end then move it up to where it belongs
    void push(int time, int netId, char val) {
        if (count == cap)
            grow();

        Event e;
        e.time = time;
        e.netId = netId;
        e.val = val;
        e.seq = nextSeq;
        nextSeq++;

        heap[count] = e;
        count++;
        siftUp(count - 1);
    }

    //take the top event and put the last one at the top and move it down, on an empty queue it prints an error and returns an event with time -1
    Event pop() {
        if (count == 0) {
            cout << "error: pop on empty event queue" << endl;
            Event bad;
            bad.time = -1;
            bad.seq = -1;
            bad.netId = -1;
            bad.val = 'X';
            return bad;
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
    Event peek() {
        if (count == 0) {
            cout << "error: peek on empty event queue" << endl;
            Event bad;
            bad.time = -1;
            bad.seq = -1;
            bad.netId = -1;
            bad.val = 'X';
            return bad;
        }
        return heap[0];
    }

    //time of the next event, -1 if empty
    int nextTime() {
        if (count == 0)
            return -1;
        return heap[0].time;
    }

    bool isEmpty() {
        return count == 0;
    }

    int size() {
        return count;
    }

    //reset so it can be used again, the array stays allocated
    void clear() {
        count = 0;
        nextSeq = 0;
    }
};


//complexity:
//push O(log k), pop O(log k)
//peek, nextTime, isEmpty, size, clear O(1)
//k events in a whole simulation take O(k log k)
//space O(k)
#endif