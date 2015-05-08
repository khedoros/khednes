#ifndef UTIL_H
#define UTIL_H
#include<stdio.h>
#include<stdarg.h>
#include<vector>
#include<bitset>

template<typename T>
class RangeCheckVector : public std::vector<T> {
public:
    T& operator[](int i) {
        return std::vector<T>::at(i);
    }

    const T& operator[](int i) const {
        return std::vector<T>::at(i);
    }
};

#ifdef DEBUG
#define Vect RangeCheckVector
#else
#define Vect std::vector
#endif

class util {
public:
    static int debug(int orig, const char * fmt,...);
    static int debug(const char * fmt,...);
    static void toggle_log(int origin);
    static bool is_active(int i);
private:
    static std::bitset<1000> active;
    static bool init;
};

#endif
