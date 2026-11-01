#ifndef DYNARRAY_HPP
#define DYNARRAY_HPP

//our own replacement for std::vector
template <class T>
class DynArray {
private:
    T*  data_;   //A pointer to the first element of a heap allocated array of T
    int size_;   //how much room is actually used by the array.
    int cap_;    //how many elements the array has has room for.

public:

    //RULE OF THREE (Contructor, Destructor, and Copy Assigment)

    DynArray();                                 //empty array, no allocation
    DynArray(const DynArray& other);            //deep copy
    DynArray& operator=(const DynArray& other); //deep copy
    ~DynArray();                                //delete

    void push_back(const T& value);             //Append at the end O(1)
    int  size()  const;                         //returns SIZE of array
    bool empty() const;                         //checks if array is empty i.e. size is zero

    //we have 2 versions const and non const for a[3] = 7 we use non const but for const arrays we use a[3] so we refer to cont
    T& operator[](int i);                 //no bounds checking, for speed (callers must stay within [0, size))
    const T& operator[](int i) const;
};

//empty array: no buffer yet, so an unused DynArray costs no heap memory.
template <class T>
DynArray<T>::DynArray() : data_(nullptr), size_(0), cap_(0) {}

//deep copy: allocate our OWN buffer and copy every element, so the two arrays never share memory. Capacity is exactly the size copied not to waste memory, the next push_back will grow it if needed.
template <class T>
DynArray<T>::DynArray(const DynArray& other)
    : data_(nullptr), size_(0), cap_(0) {
    if (other.size_ > 0) {
        data_ = new T[other.size_];
        cap_  = other.size_;
        for (int i = 0; i < other.size_; i++) data_[i] = other.data_[i];
        size_ = other.size_;
    }
}

//copy assignment: build the new buffer FIRST, then free the old one, so a failed allocation leaves this object untouched.
template <class T>
DynArray<T>& DynArray<T>::operator=(const DynArray& other) {
    if (this == &other) return *this;      //guard against a = a
    T* nd = nullptr;
    if (other.size_ > 0) {
        nd = new T[other.size_];
        for (int i = 0; i < other.size_; i++) nd[i] = other.data_[i];
    }
    delete[] data_;
    data_ = nd;
    size_ = other.size_;
    cap_  = other.size_;
    return *this;
}

//free the heap buffer (delete[] on a null pointer is safe).
template <class T>
DynArray<T>::~DynArray() { delete[] data_; }

//when full, allocate a buffer of double the capacity (4, 8, 16, ...), store the new value in it, copy the old elements across, and only then free the old buffer. 
//Most pushes are cheap: write one value into a free slot, which is O(1). Occasionally the buffer is full, and that one push costs O(size) because every element must be copied. So a single push is O(1) in the common case and O(n) in the worst case.
template <class T>
void DynArray<T>::push_back(const T& value) {
    if (size_ == cap_) {
        int newCap = (cap_ == 0) ? 4 : cap_ * 2;
        T* nd = new T[newCap];
        nd[size_] = value;                         //value still valid here
        for (int i = 0; i < size_; i++) nd[i] = data_[i]; //O(n)
        delete[] data_;
        data_ = nd;
        cap_  = newCap;
        size_++;
    } else {
        data_[size_++] = value;
    }
}

//getter
template <class T>
int DynArray<T>::size() const { return size_; }

//check if empty
template <class T>
bool DynArray<T>::empty() const { return size_ == 0; }

template <class T>
T& DynArray<T>::operator[](int i) { return data_[i]; }

template <class T>
const T& DynArray<T>::operator[](int i) const { return data_[i]; }

#endif