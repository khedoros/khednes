#pragma once
#include<stdio.h>
#include<stdarg.h>
#include<vector>
#include<bitset>
#include<string>

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

enum addr_mode {
    inv, //invalid operation
    acc, //accumulator
    imp, //implied
    imm, //immediate
    abso, //absolue
    zpa, //zero-page absolute
    rel, //relative
    absx,//absolute, x-indexed
    absy,//absolute, y-indexed
    zpx, //zero-page absolute, x-indexed
    zpy, //zero-page absolute, y-indexed
    ind, //indirect
    zpix,//zero-page indirect, x-pre-indexed
    zpiy //zero-page indirect, y-post-indexed
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
    static std::string inst_string(int byte0, int byte1=-1, int byte2=-1);

    static const std::string inst_names[];
    static const addr_mode   inst_types[];
private:
    static std::bitset<1000> active;
    static bool init;
};

