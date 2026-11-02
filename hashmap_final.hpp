// =============================================================================
// hashmap.hpp -- Milestone 1: B4 Event-Driven Gate-Level Logic Simulator
// Simple hash map: string key -> value, using separate chaining.
// =============================================================================


#ifndef HASHMAP_HPP
#define HASHMAP_HPP

//constants of the FNV-1a hash 
const unsigned int FNV_OFFSET = 2166136261u;
const unsigned int FNV_PRIME = 16777619u;

//free helper functions 

//returns true if both strings are identical. Needed because different names can land in the same bucket, so after the hash picks
//the bucket we still compare names to find the exact key. Cost: O(n) depending on the length of the string.
bool check_samename(const char* a, const char* b)
{
    while (*a != '\0' && *a == *b)   //stop at the end of either string or at the first different letter
    {
        a++;
        b++;
    }
    return *a == *b;   //equal only if both stopped at '\0' together
}

//makes a new copy of a string so each node owns its key (the caller's buffer can change or disappear later).
//the caller must delete[] the copy (the Node destructor does it). Cost: O(n) depending on the length of the string.
char* copy_name(const char* s)
{
    int n = 0;
    while (s[n] != '\0')   //count the letters before the '\0'
    {
        n++;
    }
    char* copy = new char[n + 1];   //+1 for the '\0'
    for (int i = 0; i <= n; i++)    //<= n also copies the '\0'
    {
        copy[i] = s[i];
    }
    return copy;
}

//turns a string into a number (FNV-1a hash): the same string always gives the same number and similar names give very different
//numbers.
unsigned int hashingfunction(const char* s)
{
    unsigned int h = FNV_OFFSET;
    while (*s != '\0')
    {
        h ^= (unsigned char)(*s);   //mix the letter in (unsigned char so it is never negative)
        h *= FNV_PRIME;             //spread it over all the bits
        s++;
    }
    return h;
}

template <typename T>
class HashMap {
private:
    //one entry of a bucket's linked list
    struct Node
    {
        char* key;     //our own copy of the name
        T     value;   //what the name maps to
        Node* next;    //next node in the same bucket (nullptr at the end)
        Node(const char* k, const T& v, Node* n) : key(copy_name(k)), value(v), next(n) {}
        ~Node()
        {
            delete[] key;   //free the copy made by copy_name
        }
    };

    Node** buckets;   //array of list heads
    int    capacity;  //number of buckets
    int    count;     //number of stored entries

    //which bucket the key belongs too
    int indexFor(const char* key) const
    {
        return hashingfunction(key) % capacity;
    }

    //doubles the number of buckets and moves every node to its new bucket (the bucket of a key depends on capacity, so every node
    //must be placed again). Nodes are moved, not copied. Cost: O(n), but it happens rarely so put stays O(1) amortized.
    void rehash()
    {
        int newcapacity = capacity * 2;
        Node** newbuckets = new Node * [newcapacity]();   //() sets all to nullptr

        for (int i = 0; i < capacity; i++)
        {
            Node* currentnode = buckets[i];
            while (currentnode != nullptr)
            {
                Node* nextnode = currentnode->next;                            //save next first because we overwrite it below
                int idx = hashingfunction(currentnode->key) % newcapacity;     //bucket in the bigger array
                currentnode->next = newbuckets[idx];                           //push the node on the front of that bucket
                newbuckets[idx] = currentnode;
                currentnode = nextnode;
            }
        }
        delete[] buckets;   //only the old array, the nodes now live in newbuckets
        buckets = newbuckets;
        capacity = newcapacity;
    }

    //copying is disabled so two maps never delete the same nodes
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

public:
    //creates an empty map. The minimum is 8 buckets so a 0 or negative capacity can never make an invalid array.
    // we made the defult value 16 as in number of buckets we start with is 16 
    HashMap(int initialCapacity = 16) : buckets(nullptr), capacity(initialCapacity < 8 ? 8 : initialCapacity), count(0)
    {
        buckets = new Node * [capacity]();   //() sets all to nullptr (all buckets start empty)
    }

    ~HashMap()
    {
        clearnodes();
        delete[] buckets;
    }

    //adds a key with its value, or just updates the value if the key already exists. 
    bool put(const char* key, const T& value)
    {
        int idx = indexFor(key);

        //key already there so we are just updating the value
        for (Node* currentnode = buckets[idx]; currentnode != nullptr; currentnode = currentnode->next)
        {
            if (check_samename(currentnode->key, key))
            {
                currentnode->value = value;
                return false;
            }
        }

        //keeps the load factor at or below 0.75 so the chains stay short
        if ((count + 1) * 4 > capacity * 3)
        {
            rehash();
            idx = indexFor(key);   //capacity changed so the bucket changes too
        }

        buckets[idx] = new Node(key, value, buckets[idx]);   //add at the front of the bucket (O(1))
        count++;
        return true;
    }

    //returns a pointer to the value of the key, or nullptr if the key is missing. Cost: O(1) on average.
    T* get(const char* key) const
    {
        for (Node* currentnode = buckets[indexFor(key)]; currentnode != nullptr; currentnode = currentnode->next)
        {
            if (check_samename(currentnode->key, key))
            {
                return &currentnode->value;
            }
        }
        return nullptr;
    }

    //true if the key is in the map
    bool contains(const char* key) const
    {
        return get(key) != nullptr;
    }

    //removes the key and frees its node. Returns true if it was found, false if the key was missing. Cost: O(1) on average.
    bool removekey(const char* key)
    {
        int idx = indexFor(key);
        Node* previousnode = nullptr;   //node before currentnode, needed to reconnect the list around the removed node
        for (Node* currentnode = buckets[idx]; currentnode != nullptr; previousnode = currentnode, currentnode = currentnode->next)
        {
            if (check_samename(currentnode->key, key))
            {
                if (previousnode == nullptr) buckets[idx] = currentnode->next;         //it was the first node of the bucket
                else                         previousnode->next = currentnode->next;   //skip over it
                delete currentnode;
                count--;
                return true;
            }
        }
        return false;
    }

    //deletes all nodes but keeps the bucket array, so the map is empty and can be used again. Cost: O(capacity + n).
    void clearnodes()
    {
        for (int i = 0; i < capacity; i++)
        {
            Node* currentnode = buckets[i];
            while (currentnode != nullptr)
            {
                Node* nextnode = currentnode->next;   //save next first because the node is deleted
                delete currentnode;
                currentnode = nextnode;
            }
            buckets[i] = nullptr;
        }
        count = 0;
    }

    //number of stored keys
    int  size() const { return count; }

    //true if no keys are stored
    bool empty() const { return count == 0; }

    //number of buckets 
    int  bucketCount() const { return capacity; }
};

#endif


