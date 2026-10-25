// Defines common types, generic helper utilities,
// and foundational data structures shared by the scanning and data-processing modules.
#ifndef _Base_H_
#define _Base_H_

#include <iostream>

using namespace std;


inline unsigned int get_size(const char* x) {
    unsigned int i = 0;
    while (*(x + i) != '\0')
        ++i;
    return i;
}

template <typename T>
inline unsigned int get_size(const T &x) {
    return sizeof(x) / sizeof(x[0]);
}

#ifdef _WIN32
template <typename T>
inline unsigned int get_size(T* const &x) {
    return _msize(x) / sizeof(*x);
}
#else
#include "malloc.h"
template <typename T>
inline unsigned int get_size(T* const &x) {
    return malloc_usable_size(x) / sizeof(*x);
}
#endif

inline short compare(const char* x1, const char* x2) {
    auto sz1 = get_size(x1), sz2 = get_size(x2);
    if (sz1 > sz2)
        return 1;
    if (sz1 < sz2)
        return -1;
    
    unsigned int i = 0;
    while (i < sz1) {
        if (*(x1 + i) > *(x2 + i))
            return 1;
        if (*(x1 + i) < *(x2 + i))
            return -1;
        ++i;
    }

    return 0;
}


template <typename T>
inline char _is_ptr(T* const &v) {
    return 'd';
}

inline int _is_ptr(...) {
    return 0;
}

#define IS_PTR(p) (sizeof(_is_ptr(p)) == sizeof(char))


template <class A, class B>
struct DataPair {
public:
    DataPair(A a, B b) : _first(a), _second(b) {};
    ~DataPair() {};

    A get_first() { return _first; };
    B get_second() { return _second; };
    void set_first(A v) { _first = v; };
    void set_second(B v) { _second = v; };

    friend std::ostream& operator << (std::ostream& out, DataPair<A, B>& pair) {
        cout << "(" << pair.get_first() << ", " << pair.get_second() << ")";
        return out;
    };
private:
    A _first;
    B _second;
};


#define ROUND(x, d) (round((x) * pow(10, (d))) / pow(10, (d)))


#include "base.cpp"
#endif