#ifndef HASHSET_H
#define HASHSET_H

#include "DynArray.hpp"

/*
 This class is an active gate deduplication mechanism.
 When a gate's input changes (wire_1 = 0 -> wire_1 = 1) the gate is flagged for
 re-evaluation. This set makes sure a gate is scheduled only once per time step.
 It stores gate IDS: Graph keeps gates by value in a DynArray, and
 the gates move in memory when that array grows, so pointers would dangle.
*/
class GateHashSet
{
private:
    DynArray< DynArray<int> > table;      //separate chaining: one small array per bucket
    int capacity;                         //number of buckets
    int num_elements;                     //number of stored ids
    static constexpr float MAX_LOAD_FACTOR = 0.75f;   //keeps chains short so ops stay O(1)

    //Fibonacci hashing: multiply by 2^64/golden ratio, then use the HIGH bits (they are well mixed)
    int hash_function(int id, int table_capacity) const {
        unsigned long long x = (unsigned long long)(unsigned int)id * 11400714819323198485ULL;
        return (int)((x >> 32) % (unsigned long long)table_capacity);
    }

    int hash_function(int id) const { return hash_function(id, capacity); }

    //called when the load factor would be exceeded: build a bigger table and move every id into it
    void rehash(int new_capacity) {
        DynArray< DynArray<int> > new_table;
        for (int i = 0; i < new_capacity; ++i)
            new_table.push_back(DynArray<int>());

        for (int i = 0; i < table.size(); ++i) {
            const DynArray<int>& bucket = table[i];
            for (int j = 0; j < bucket.size(); ++j) {
                int id = bucket[j];
                new_table[hash_function(id, new_capacity)].push_back(id);
            }
        }
        table = new_table;        //deep copy (rule of three in DynArray)
        capacity = new_capacity;
    }

public:
    explicit GateHashSet(int initial_capacity = 101)
        : capacity(initial_capacity < 8 ? 8 : initial_capacity), num_elements(0) {
        for (int i = 0; i < capacity; ++i)
            table.push_back(DynArray<int>());
    }

    //true if the gate id is in the set. O(1) average.
    bool contains(int id) const {
        if (id < 0) return false;
        const DynArray<int>& bucket = table[hash_function(id)];
        for (int i = 0; i < bucket.size(); ++i)
            if (bucket[i] == id) return true;
        return false;
    }

    //adds the id. Returns false if it was already there (so the caller knows it was a duplicate). O(1) amortized.
    bool insert(int id) {
        if (id < 0 || contains(id)) return false;
        if ((float)(num_elements + 1) / capacity > MAX_LOAD_FACTOR)
            rehash(capacity * 2 + 1);
        table[hash_function(id)].push_back(id);
        num_elements++;
        return true;
    }

    //removes the id. Returns false if it was not there. O(1) average.
    bool remove(int id) {
        if (id < 0) return false;
        DynArray<int>& bucket = table[hash_function(id)];
        for (int i = 0; i < bucket.size(); ++i) {
            if (bucket[i] == id) {
                bucket[i] = bucket.back();   //overwrite with the last id, then drop the last slot
                bucket.pop_back();           //(order inside a bucket does not matter)
                num_elements--;
                return true;
            }
        }
        return false;
    }

    //empties the set but keeps the bucket arrays, so it is cheap to reuse every time step.
    //O(capacity) when it holds something, O(1) when it is already empty.
    void clear() {
        if (num_elements == 0) return;
        for (int i = 0; i < capacity; ++i)
            table[i].clear();
        num_elements = 0;
    }

    int   size() const { return num_elements; }
    bool  empty() const { return num_elements == 0; }
    int   get_capacity() const { return capacity; }
    float load_factor() const { return (float)num_elements / capacity; }
};

#endif