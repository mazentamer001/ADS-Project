#pragma once
#include "DynArray"
#include <cstddef>

struct Gate;

/*
This  class  is  an active gate deduplication mechanism.
In case  of  gate  drivers vlaue change(wire_1 = 0 -> wire_1 = 1)
flags  are then   sent  for  gate  revluation
this  calss  ensures  no  duplicate  gate  reevauluation.
*/
class Gatehashset
{
    private :
   DynArray<DynArray<Gate*>> table;
    std:: size_t capacity; 
  
     std:: size_t num_elements;
    constexpr static  float MAX_LOAD_FACTOR = 0.75f; // load factor  to  ensure  O(1) complexity 
  
        //fibo hashfunction
   std::size_t hash_function(Gate* gate, std::size_t table_capacity) const {
       // casting ptr into  same  format bit  integer
    std::size_t addr = reinterpret_cast<std::size_t>(gate);
        return ((addr >> 4) * 11400714819323198485ULL) % table_capacity;
    }

    //  version for normal lookups
    std::size_t hash_function(Gate* gate) const {
        return hash_function(gate, capacity);
    }
    // in case of exceeding the load factor
    void rehash(std :: size_t new_capacity){
         DynArray<DynArray<Gate*>> new_table;
        for (std::size_t i = 0; i < new_capacity; ++i) {
            new_table.push_back(DynArray<Gate*>());
        }

        for (int i = 0; i < table.size(); ++i) {
            const auto& bucket = table[i];
            for (int j = 0; j < bucket.size(); ++j) {
                Gate* gate = bucket[j];
                std::size_t new_index = hash_function(gate, new_capacity);
                new_table[static_cast<int>(new_index)].push_back(gate);
            }
        }

        table = new_table; 
        capacity = new_capacity;
    }
    public:
   explicit GateHashSet(std::size_t initial_capacity = 101) 
        : capacity(initial_capacity), num_elements(0) {
        for (std::size_t i = 0; i < capacity; ++i) {
            table.push_back(DynArray<Gate*>());
        }
    }
   
        bool contains(Gate *gate ){
         if (!gate)return false;
         std::size_t index = hash_function(gate);
       const auto& bucket = table[static_cast<int>(index)];

        for (int i = 0; i < bucket.size(); ++i) {
            if (bucket[i] == gate) return true;
        }
        return false;
    }
    bool insert(Gate *gate ){
        if(!gate||contains(gate))return false;
        if (static_cast<float>(num_elements + 1) / capacity > MAX_LOAD_FACTOR) {
            rehash(capacity * 2 + 1);
           std:: size_t index=hash_function(gate);
           table[static_cast<int>(index)].push_back(gate);
            num_elements++
            return true;
        }
        
    }
    bool remove(Gate*gate ){
        if(!gate||!contains(gate))return false;
        std::size_t index = hash_function(gate);
       auto& bucket = table[static_cast<int>(index)];

        int target_idx = -1;
        for (int i = 0; i < bucket.size(); ++i) {
            if (bucket[i] == gate) {
                target_idx = i;
                break;
            }
        }

        if (target_idx == -1) return false;

        DynArray<Gate*> new_bucket;
        for (int i = 0; i < bucket.size(); ++i) {
            if (i != target_idx) {
                new_bucket.push_back(bucket[i]);
            }
        }

        bucket = new_bucket;
        num_elements--;
        return true;
    }
    
    std :: size_t size() const { return num_elements; }
    std :: size_t get_capacity() const { return capacity; }
    float load_factor() const { return static_cast<float>(num_elements) / capacity; }


/*void swap(DynArray& other) {
    T* tmp_data = data_; data_ = other.data_; other.data_ = tmp_data;
    int tmp_sz = size_; size_ = other.size_; other.size_ = tmp_sz;
    int tmp_cp = cap_; cap_ = other.cap_; other.cap_ = tmp_cp;
}
void pop_back() { if (size_ > 0) size_--; }
void clear() { size_ = 0; }
*/






};
