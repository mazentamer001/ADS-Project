#pragma once
#include <cstddef>

struct Gate;


class Gatehashset
{
    private :
    std:: vector< std:: vector<Gate>> table;
    std:: size_t capacity; 
  
     std:: size_t num_elements;
    constexpr static  float MAX_LOAD_FACTOR = 0.75f;
  
        //fibo hashfunction
   std::size_t hash_function(Gate* gate, std::size_t table_capacity) const {
       // casting ptr into  same  format  integer
    std::size_t addr = reinterpret_cast<std::size_t>(gate);
        return ((addr >> 4) * 11400714819323198485ULL) % table_capacity;
    }

    //  version for normal lookups
    std::size_t hash_function(Gate* gate) const {
        return hash_function(gate, capacity);
    }
    
    void rehash(std :: size_t new_capacity){
          vector<vector<Gate>> new_table(new_capacity);
          for(const auto&container:table){
          for(Gate* gate: container){
            size_t new_index=hash_function(gate, new_capacity);
            new_table[new_index].push_back(gate);
          }

          }
          table.swap(new_table);
        capacity = new_capacity;
    }
    public:
    Gatehashset(std :: size_t initial_capacity = 101):capacity( initial_capacity),num_elements(0) 
        table.resize(capacity);{}
   
        bool contains(Gate *gate ){
         if (!gate)return false;
         std::size_t index = hash_function(gate);
         for(Gate *item :table[index]){
            if (item == gate) return true;
         }
         return true 
        }
    bool insert(Gate *gate ){
        if(!gate||contains(gate))return false;
        if (static_cast<float>(num_elements + 1) / capacity > MAX_LOAD_FACTOR) {
            rehash(capacity * 2 + 1);
           std:: size_t index=hash_function(gate);
           table[index].push_back(gate);
            num_elements++
            return true;
        }
        
    }
    bool remove(Gate*gate ){
        if(!gate||!contains(gate))return false;
        std:: size_t index=hash_function(gate);
        for (auto it = container.begin(); it != container.end(); ++it) {
            if (*it == gate) {
                bucket.erase(it);
                num_elements--;
                return true;
            }
        }
        return false 
    }
    void clear() {
        for (auto& bucket : table) {
            bucket.clear(); // Clears elements, retains memory capacity
        }
        num_elements = 0;
    }
    
    std :: size_t size() const { return num_elements; }
    std :: size_t get_capacity() const { return capacity; }
    bool empty() const { return num_elements == 0; }
    float load_factor() const { return static_cast<float>(num_elements) / capacity; }









};